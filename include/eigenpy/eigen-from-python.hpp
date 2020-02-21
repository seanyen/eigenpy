//
// Copyright (c) 2014-2020 CNRS INRIA
//

#ifndef __eigenpy_eigen_from_python_hpp__
#define __eigenpy_eigen_from_python_hpp__

#include "eigenpy/fwd.hpp"
#include "eigenpy/numpy-type.hpp"
#include "eigenpy/eigen-allocator.hpp"
#include "eigenpy/scalar-conversion.hpp"

#include <boost/python/converter/rvalue_from_python_data.hpp>

namespace boost { namespace python { namespace converter {

  /// \brief Template specialization of rvalue_from_python_data
  template<typename Derived>
  struct rvalue_from_python_data<Eigen::MatrixBase<Derived> const & >
  : rvalue_from_python_storage<Derived const & >
  {
    typedef Eigen::MatrixBase<Derived> const & T;
    
# if (!defined(__MWERKS__) || __MWERKS__ >= 0x3000) \
&& (!defined(__EDG_VERSION__) || __EDG_VERSION__ >= 245) \
&& (!defined(__DECCXX_VER) || __DECCXX_VER > 60590014) \
&& !defined(BOOST_PYTHON_SYNOPSIS) /* Synopsis' OpenCXX has trouble parsing this */
    // This must always be a POD struct with m_data its first member.
    BOOST_STATIC_ASSERT(BOOST_PYTHON_OFFSETOF(rvalue_from_python_storage<T>,stage1) == 0);
# endif
    
    // The usual constructor
    rvalue_from_python_data(rvalue_from_python_stage1_data const & _stage1)
    {
      this->stage1 = _stage1;
    }
    
    // This constructor just sets m_convertible -- used by
    // implicitly_convertible<> to perform the final step of the
    // conversion, where the construct() function is already known.
    rvalue_from_python_data(void* convertible)
    {
      this->stage1.convertible = convertible;
    }
    
    // Destroys any object constructed in the storage.
    ~rvalue_from_python_data()
    {
      if (this->stage1.convertible == this->storage.bytes)
        static_cast<Derived *>((void *)this->storage.bytes)->~Derived();
    }
  };

  /// \brief Template specialization of rvalue_from_python_data
  template<typename Derived>
  struct rvalue_from_python_data<Eigen::EigenBase<Derived> const & >
  : rvalue_from_python_storage<Derived const & >
  {
    typedef Eigen::EigenBase<Derived> const & T;
    
# if (!defined(__MWERKS__) || __MWERKS__ >= 0x3000) \
&& (!defined(__EDG_VERSION__) || __EDG_VERSION__ >= 245) \
&& (!defined(__DECCXX_VER) || __DECCXX_VER > 60590014) \
&& !defined(BOOST_PYTHON_SYNOPSIS) /* Synopsis' OpenCXX has trouble parsing this */
    // This must always be a POD struct with m_data its first member.
    BOOST_STATIC_ASSERT(BOOST_PYTHON_OFFSETOF(rvalue_from_python_storage<T>,stage1) == 0);
# endif
    
    // The usual constructor
    rvalue_from_python_data(rvalue_from_python_stage1_data const & _stage1)
    {
      this->stage1 = _stage1;
    }
    
    // This constructor just sets m_convertible -- used by
    // implicitly_convertible<> to perform the final step of the
    // conversion, where the construct() function is already known.
    rvalue_from_python_data(void* convertible)
    {
      this->stage1.convertible = convertible;
    }
    
    // Destroys any object constructed in the storage.
    ~rvalue_from_python_data()
    {
      if (this->stage1.convertible == this->storage.bytes)
        static_cast<Derived *>((void *)this->storage.bytes)->~Derived();
    }
  };

} } }

namespace eigenpy
{
  template<typename MatType>
  struct EigenFromPy
  {
    
    static bool isScalarConvertible(const int np_type)
    {
      if(NumpyEquivalentType<typename MatType::Scalar>::type_code == np_type)
        return true;
      
      switch(np_type)
      {
        case NPY_INT:
          return FromTypeToType<int,typename MatType::Scalar>::value;
        case NPY_LONG:
          return FromTypeToType<long,typename MatType::Scalar>::value;
        case NPY_FLOAT:
          return FromTypeToType<float,typename MatType::Scalar>::value;
        case NPY_CFLOAT:
          return FromTypeToType<std::complex<float>,typename MatType::Scalar>::value;
        case NPY_DOUBLE:
          return FromTypeToType<double,typename MatType::Scalar>::value;
        case NPY_CDOUBLE:
          return FromTypeToType<std::complex<double>,typename MatType::Scalar>::value;
        case NPY_LONGDOUBLE:
          return FromTypeToType<long double,typename MatType::Scalar>::value;
        case NPY_CLONGDOUBLE:
          return FromTypeToType<std::complex<long double>,typename MatType::Scalar>::value;
        default:
          return false;
      }
    }
    
    /// \brief Determine if pyObj can be converted into a MatType object
    static void* convertible(PyArrayObject* pyArray)
    {
      if(!PyArray_Check(pyArray))
        return 0;
      
      if(!isScalarConvertible(EIGENPY_GET_PY_ARRAY_TYPE(pyArray)))
        return 0;

      if(MatType::IsVectorAtCompileTime)
      {
        const Eigen::DenseIndex size_at_compile_time
        = MatType::IsRowMajor
        ? MatType::ColsAtCompileTime
        : MatType::RowsAtCompileTime;
        
        switch(PyArray_NDIM(pyArray))
        {
          case 0:
            return 0;
          case 1:
          {
            if(size_at_compile_time != Eigen::Dynamic)
            {
              // check that the sizes at compile time matche
              if(PyArray_DIMS(pyArray)[0] == size_at_compile_time)
                return pyArray;
              else
                return 0;
            }
            else // This is a dynamic MatType
              return pyArray;
          }
          case 2:
          {
            // Special care of scalar matrix of dimension 1x1.
            if(PyArray_DIMS(pyArray)[0] == 1 && PyArray_DIMS(pyArray)[1] == 1)
            {
              if(size_at_compile_time != Eigen::Dynamic)
              {
                if(size_at_compile_time == 1)
                  return pyArray;
                else
                  return 0;
              }
              else // This is a dynamic MatType
                return pyArray;
            }
            
            if(PyArray_DIMS(pyArray)[0] > 1 && PyArray_DIMS(pyArray)[1] > 1)
            {
#ifndef NDEBUG
              std::cerr << "The number of dimension of the object does not correspond to a vector" << std::endl;
#endif
              return 0;
            }
            
            if(((PyArray_DIMS(pyArray)[0] == 1) && (MatType::ColsAtCompileTime == 1))
               || ((PyArray_DIMS(pyArray)[1] == 1) && (MatType::RowsAtCompileTime == 1)))
            {
#ifndef NDEBUG
              if(MatType::ColsAtCompileTime == 1)
                std::cerr << "The object is not a column vector" << std::endl;
              else
                std::cerr << "The object is not a row vector" << std::endl;
#endif
              return 0;
            }
            
            if(size_at_compile_time != Eigen::Dynamic)
            { // This is a fixe size vector
              const Eigen::DenseIndex pyArray_size
              = PyArray_DIMS(pyArray)[0] > PyArray_DIMS(pyArray)[1]
              ? PyArray_DIMS(pyArray)[0]
              : PyArray_DIMS(pyArray)[1];
              if(size_at_compile_time != pyArray_size)
                return 0;
            }
            break;
          }
          default:
            return 0;
        }
      }
      else // this is a matrix
      {
        if(PyArray_NDIM(pyArray) == 1) // We can always convert a vector into a matrix
        {
          return pyArray;
        }
        
        if(PyArray_NDIM(pyArray) != 2)
        {
#ifndef NDEBUG
            std::cerr << "The number of dimension of the object is not correct." << std::endl;
#endif
          return 0;
        }
       
        if(PyArray_NDIM(pyArray) == 2)
        {
          const int R = (int)PyArray_DIMS(pyArray)[0];
          const int C = (int)PyArray_DIMS(pyArray)[1];
          
          if( (MatType::RowsAtCompileTime!=R)
             && (MatType::RowsAtCompileTime!=Eigen::Dynamic) )
            return 0;
          if( (MatType::ColsAtCompileTime!=C)
             && (MatType::ColsAtCompileTime!=Eigen::Dynamic) )
            return 0;
        }
      }
        
#ifdef NPY_1_8_API_VERSION
      if(!(PyArray_FLAGS(pyArray)))
#else
      if(!(PyArray_FLAGS(pyArray) & NPY_ALIGNED))
#endif
      {
#ifndef NDEBUG
        std::cerr << "NPY non-aligned matrices are not implemented." << std::endl;
#endif
        return 0;
      }
      
      return pyArray;
    }
 
    /// \brief Allocate memory and copy pyObj in the new storage
    static void construct(PyObject* pyObj,
                          bp::converter::rvalue_from_python_stage1_data* memory)
    {
      PyArrayObject * pyArray = reinterpret_cast<PyArrayObject*>(pyObj);
      assert((PyArray_DIMS(pyArray)[0]<INT_MAX) && (PyArray_DIMS(pyArray)[1]<INT_MAX));
      
      void* storage = reinterpret_cast<bp::converter::rvalue_from_python_storage<MatType>*>
                     (reinterpret_cast<void*>(memory))->storage.bytes;
      
      EigenAllocator<MatType>::allocate(pyArray,storage);

      memory->convertible = storage;
    }
    
    static void registration()
    {
      bp::converter::registry::push_back
      (reinterpret_cast<void *(*)(_object *)>(&EigenFromPy::convertible),
       &EigenFromPy::construct,bp::type_id<MatType>());
    }
  };
  
  template<typename MatType>
  struct EigenFromPyConverter
  {
    static void registration()
    {
      EigenFromPy<MatType>::registration();

      // Add also conversion to Eigen::MatrixBase<MatType>
      typedef Eigen::MatrixBase<MatType> MatrixBase;
      EigenFromPy<MatrixBase>::registration();

      // Add also conversion to Eigen::EigenBase<MatType>
      typedef Eigen::EigenBase<MatType> EigenBase;
      EigenFromPy<EigenBase>::registration();
    }
  };

  template<typename MatType>
  struct EigenFromPy< Eigen::MatrixBase<MatType> > : EigenFromPy<MatType>
  {
    typedef EigenFromPy<MatType> EigenFromPyDerived;
    typedef Eigen::MatrixBase<MatType> Base;

    static void registration()
    {
      bp::converter::registry::push_back
      (reinterpret_cast<void *(*)(_object *)>(&EigenFromPy::convertible),
       &EigenFromPy::construct,bp::type_id<Base>());
    }
  };
    
  template<typename MatType>
  struct EigenFromPy< Eigen::EigenBase<MatType> > : EigenFromPy<MatType>
  {
    typedef EigenFromPy<MatType> EigenFromPyDerived;
    typedef Eigen::EigenBase<MatType> Base;

    static void registration()
    {
      bp::converter::registry::push_back
      (reinterpret_cast<void *(*)(_object *)>(&EigenFromPy::convertible),
       &EigenFromPy::construct,bp::type_id<Base>());
    }
  };

#if EIGEN_VERSION_AT_LEAST(3,2,0)
  // Template specialization for Eigen::Ref
  template<typename MatType>
  struct EigenFromPyConverter< eigenpy::Ref<MatType> >
  {
    static void registration()
    {
      bp::converter::registry::push_back
      (reinterpret_cast<void *(*)(_object *)>(&EigenFromPy<MatType>::convertible),
       &EigenFromPy<MatType>::construct,bp::type_id<MatType>());
    }
  };

#endif
}

#endif // __eigenpy_eigen_from_python_hpp__
