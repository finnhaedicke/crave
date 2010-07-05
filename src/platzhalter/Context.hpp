#pragma once

#include "bitsize_traits.hpp"

#include <boost/proto/core.hpp>
#include <boost/proto/debug.hpp>
#include <boost/proto/context/callable.hpp>
#include <boost/proto/context/null.hpp>
#include <boost/function.hpp>

#include <MetaSolver.hpp>
#include <logic/QF_BV.hpp>
#include <tools/Convert_QF_BV.hpp>

#include <map>
#include <vector>
#include <string>
#include <stdexcept>

namespace platzhalter {
  namespace proto = boost::proto;

  struct metaSMT_Context
    : proto::callable_context< metaSMT_Context, proto::null_context >
  {

    metaSMT_Context(std::string const & solvername="SolverBoolector") 
    : _solver(new metaSMT::MetaSolver())
    , _logic(_solver->logic<metaSMT::QF_BV>(solvername))
    {}

    ~metaSMT_Context() {
      delete _solver;
    }

    typedef metaSMT::Expression result_type;


    template<typename value_type>
    result_type operator() (proto::tag::terminal, var_tag<value_type> const & tag ) {
      std::map<int, result_type>::const_iterator ite
        = _variables.find(tag.id);
      if ( ite != _variables.end() ) {
        return ite->second;
      } else {
        std::ostringstream buf;
        buf << "var_" << tag.id;
        unsigned width=bitsize_traits<value_type>::nbits;
        //std::cout << "creating " << buf.str() << ", width= " << width << std::endl;
        result_type var = _logic->bitvec(buf.str(), width);
        _variables.insert( std::make_pair(tag.id, var) );
        return var;
      }
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::equal_to, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->equal(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::not_equal_to, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->nequal(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::less_equal, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->bvule(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::less, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->bvult(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::greater, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->bvugt(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }
    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::greater_equal, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->bvuge(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::logical_and, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->bvand(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::logical_or, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->bvor(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::plus, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->bvadd(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::minus, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->bvsub(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::modulus, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->bvurem(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::divides, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->bvudiv(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::shift_left, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->bvshl(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }


    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::multiplies, Expr1 const & e1, Expr2 const &  e2) {
      return _logic->bvmul(
          proto::eval( e1, *this )
        , proto::eval( e2, *this )
      );
    }

    template< typename Expr1, typename Expr2>
    result_type operator() (proto::tag::assign, Expr1 const & e1, Expr2 const &  e2) {
      //std::cout << "eval assign" << std::endl;
      result_type expr1, expr2;
      expr1 = proto::eval( e1, *this );
      expr2 = proto::eval( e2, *this );

      boost::function0<result_type> f = getID( _logic->equal(expr1, expr2) );
      _lazy[proto::value(e1).id] = f;
      
      return _logic->bit1();
    }

    result_type operator() (proto::tag::terminal t, randomize_tag const & tag )
    {
      _lazy.erase(tag.id);
      return _logic->bit1();
    }

    struct getID {
      getID( result_type id) : id(id) {}
      result_type id;
      result_type operator() () { return id; }
    };

    template<typename Integer>
    struct getLazyReference{
      getLazyReference(Integer const & i, metaSMT::QF_BV* logic, result_type var) 
      : _i (i), _logic(logic), _var(var)
      {}
      result_type operator() () {
        return _logic->equal(_var, _logic->bvuint(_i, bitsize_traits<Integer>::nbits) );
      }
      Integer const & _i;
      metaSMT::QF_BV* _logic;
      result_type _var;
    };

    template< typename Integer>
    result_type operator() (proto::tag::terminal t, ref_tag<Integer> const & ref) {
      unsigned width=bitsize_traits<Integer>::nbits;

      std::map<int, result_type>::const_iterator ite
        = _variables.find(ref.id);
      if ( ite != _variables.end() ) {
        return ite->second;
      } else {
        result_type var = (*this)(t, static_cast<var_tag<Integer> >(ref));
        boost::function0<result_type> f = getLazyReference<Integer>(ref.ref, _logic, var);
        _lazy[ref.id] = f;

        return var;
      }
    }

    template< typename Integer>
    result_type operator() (proto::tag::terminal, Integer const & i) {
      unsigned width=bitsize_traits<Integer>::nbits;
      //std::cout << "creating int " << i << ", width= " << width << std::endl;
      return _logic->bvuint(i, width);
    }

    template<typename Expr>
    void assertion (Expr e) {
      check(e);
      _solver->addAssertion( proto::eval(e, *this) );
    }

    bool solve () {
      for (
        std::map<unsigned, boost::function0<result_type> > ::const_iterator 
        ite = _lazy.begin();
        ite != _lazy.end(); ++ite) {
        _solver->addAssumption( (ite->second)() );
      }
      return _solver->solve( );
    }

    template<typename T>
    T read ( Variable<T> const & v) {
      //std::cout << boost::format("read id: %d\n")% v.id();
      std::map<int, result_type>::const_iterator ite
        = _variables.find(v.id());
      assert ( ite != _variables.end() );
      std::string solution = _solver->readAssignment(ite->second);
      return metaSMT::bits2Cu(solution);
    }


    private:
      metaSMT::MetaSolver* _solver;
      metaSMT::QF_BV*      _logic;
      std::map<int, result_type> _variables;
      std::map<unsigned, boost::function0<result_type> > _lazy;

  }; // metaSMT_Context

  typedef metaSMT_Context Context;

  Context get_context(std::string const & solvername="") {
    return metaSMT_Context(solvername);
  }

  template<typename ContextT=Context>
  struct Generator {

    Generator() {};

    template<typename Expr>
    Generator(Expr expr)
    {
      (*this)(expr);
    }

    template<typename Expr>
    Generator<ContextT> & operator() (Expr expr)
    {
      //display_expr(expr);
      ctx.assertion(expr);
      return *this;
    }

    /**
     * generate a new assignment
     **/
    Generator<ContextT> & operator() () {
      if(! next() )
        throw std::runtime_error("Generator constraint unsatisfiable.");
      return *this;
    }

    bool next() {
      return ctx.solve();
    }

    /**
     * read variable "var"
     **/
    template<typename T>
    T operator[] (Variable<T> const & var) { return ctx.read(var); };

    private:
      ContextT ctx;
  };


} // namespace platzhalter
//  vim: ft=cpp:ts=2:sw=2:expandtab
