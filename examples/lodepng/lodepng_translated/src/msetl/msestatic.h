
// Copyright (c) 2015 Noah Lopez
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef MSESTATIC_H_
#define MSESTATIC_H_

#include "msepointerbasics.h"
#include "mseprimitives.h"
#include "msemsearray.h"
//#include "mseasyncshared.h"
#include <utility>
#include <unordered_set>
#include <functional>
#include <cassert>

#ifdef MSE_STATICPOINTER_RUNTIME_CHECKS_ENABLED
#include "msenorad.h"
#include <atomic>
#include "mseany.h"
#endif // MSE_STATICPOINTER_RUNTIME_CHECKS_ENABLED

#ifdef _MSC_VER
#pragma warning( push )  
#pragma warning( disable : 4100 4456 4189 )
#endif /*_MSC_VER*/

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-value"
#else /*__clang__*/
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-value"
#endif /*__GNUC__*/
#endif /*__clang__*/

/* Note that by default, MSE_STATICPOINTER_DISABLED is defined in non-debug builds. This is enacted in "msepointerbasics.h". */

#ifndef _NOEXCEPT
#define _NOEXCEPT
#endif /*_NOEXCEPT*/


namespace mse {
	namespace rsv {

		/* This macro roughly simulates constructor inheritance. */
#define MSE_STATIC_USING(Derived, Base) MSE_USING(Derived, Base)

		template<typename _Ty> class TStaticImmutableID {};

		template<typename _Ty>
		class TPlaceHolder_msestatic {};
		template<typename _Ty>
		class TPlaceHolder2_msestatic {};

#ifdef MSE_STATICPOINTER_DISABLED
		//TStaticImmutableID
		template<typename _Ty> using TStaticImmutablePointer = _Ty * ;
		template<typename _Ty> using TStaticImmutableConstPointer = const _Ty*;
		template<typename _Ty> using TStaticImmutableNotNullPointer = _Ty * ;
		template<typename _Ty> using TStaticImmutableNotNullConstPointer = const _Ty*;
		template<typename _Ty> using TStaticImmutableFixedPointer = _Ty * /*const*/; /* Can't be const qualified because standard
																		   library containers don't support const elements. */
		template<typename _Ty> using TStaticImmutableFixedConstPointer = const _Ty* /*const*/;
		template<typename _TROy> using TStaticImmutableObjBase = _TROy;
		template<typename _TROy> using TStaticImmutableObj = _TROy;

		template<typename _TROy> class TStaticImmutableOwnerPointer;

		template<typename _Ty> auto static_fptr_to(_Ty&& _X) { return std::addressof(_X); }
		template<typename _Ty> auto static_fptr_to(const _Ty& _X) { return std::addressof(_X); }

#else /*MSE_STATICPOINTER_DISABLED*/

#if defined(MSE_STATICPOINTER_RUNTIME_CHECKS_ENABLED)

		template<typename _Ty> class TWTSNoradObj;
		template<typename _Ty> class TWTSNoradPointer;
		template<typename _Ty> class TWTSNoradConstPointer;
		template<typename _Ty> class TWTSNoradNotNullPointer;
		template<typename _Ty> class TWTSNoradNotNullConstPointer;
		template<typename _Ty> class TWTSNoradFixedPointer;
		template<typename _Ty> class TWTSNoradFixedConstPointer;

		template<typename _Ty>
		class TWTSNoradPointer : public TPointer<TWTSNoradObj<_Ty>>, public StrongPointerTagBase {
		public:
			TWTSNoradPointer() : TPointer<TWTSNoradObj<_Ty>>() {}
			TWTSNoradPointer(const TWTSNoradPointer& src_cref) : TPointer<TWTSNoradObj<_Ty>>(src_cref.m_ptr) {
				if (*this) { (*(*this)).increment_refcount(); }
			}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradPointer(const TWTSNoradPointer<_Ty2>& src_cref) : TPointer<TWTSNoradObj<_Ty>>(src_cref.m_ptr) {
				if (*this) { (*(*this)).increment_refcount(); }
			}
			TWTSNoradPointer(TWTSNoradPointer&& src_ref) : TPointer<TWTSNoradObj<_Ty>>(std::forward<decltype(src_ref.m_ptr)>(src_ref.m_ptr)) {
				src_ref.m_ptr = nullptr;
			}
			TWTSNoradPointer(std::nullptr_t) : TPointer<TWTSNoradObj<_Ty>>(nullptr) {}
			virtual ~TWTSNoradPointer() {
				if (*this) { (*(*this)).decrement_refcount(); }
			}
			TWTSNoradPointer<_Ty>& operator=(const TWTSNoradPointer<_Ty>& _Right_cref) {
				if (*this) { (*(*this)).decrement_refcount(); }
				TPointer<TWTSNoradObj<_Ty>>::operator=(_Right_cref);
				if (*this) { (*(*this)).increment_refcount(); }
				return (*this);
			}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradPointer<_Ty>& operator=(const TWTSNoradPointer<_Ty2>& _Right_cref) {
				return (*this).operator=(TWTSNoradPointer(_Right_cref));
			}

			TWTSNoradObj<_Ty>& operator*() const {
				if (!((*this).m_ptr)) { MSE_THROW(primitives_null_dereference_error("attempt to dereference null pointer - mse::TTSNoradPointer")); }
				return *((*this).m_ptr);
			}
			TWTSNoradObj<_Ty>* operator->() const {
				if (!((*this).m_ptr)) { MSE_THROW(primitives_null_dereference_error("attempt to dereference null pointer - mse::TTSNoradPointer")); }
				return (*this).m_ptr;
			}

			operator bool() const { return (*this).m_ptr; }
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			operator _Ty*() const {
#ifdef NATIVE_PTR_DEBUG_HELPER1
				if (nullptr == (*this).m_ptr) {
					int q = 5; /* just a line of code for putting a debugger break point */
				}
#endif /*NATIVE_PTR_DEBUG_HELPER1*/
				return (*this).m_ptr;
			}

			/* In C++, if an object is deleted via a pointer to its base class and the base class' destructor is not virtual,
			then the (derived) object's destructor won't be called possibly resulting in resource leaks. With registered
			objects, the destructor not being called also circumvents their memory safety mechanism. */
			void norad_delete() {
				auto a = asANativePointerToTWTSNoradObj();
				(*this) = nullptr;
				delete a;
			}

		private:
			TWTSNoradPointer(TWTSNoradObj<_Ty>* ptr) : TPointer<TWTSNoradObj<_Ty>>(ptr) {
				assert(*this);
				(*(*this)).increment_refcount();
			}

			/* This function, if possible, should not be used. It is meant to be used exclusively by norad_delete<>(). */
			TWTSNoradObj<_Ty>* asANativePointerToTWTSNoradObj() const {
#ifdef NATIVE_PTR_DEBUG_HELPER1
				if (nullptr == (*this).m_ptr) {
					int q = 5; /* just a line of code for putting a debugger break point */
				}
#endif /*NATIVE_PTR_DEBUG_HELPER1*/
				return static_cast<TWTSNoradObj<_Ty>*>((*this).m_ptr);
			}

			MSE_DEFAULT_OPERATOR_AMPERSAND_DECLARATION;

			template <class Y> friend class TWTSNoradPointer;
			template <class Y> friend class TWTSNoradConstPointer;
			friend class TWTSNoradNotNullPointer<_Ty>;
		};

		template<typename _Ty>
		class TWTSNoradConstPointer : public TPointer<const TWTSNoradObj<_Ty>>, public StrongPointerTagBase {
		public:
			TWTSNoradConstPointer() : TPointer<const TWTSNoradObj<_Ty>>() {}
			TWTSNoradConstPointer(const TWTSNoradConstPointer& src_cref) : TPointer<const TWTSNoradObj<_Ty>>(src_cref.m_ptr) {
				if (*this) { (*(*this)).increment_refcount(); }
			}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradConstPointer(const TWTSNoradConstPointer<_Ty2>& src_cref) : TPointer<const TWTSNoradObj<_Ty>>(src_cref.m_ptr) {
				if (*this) { (*(*this)).increment_refcount(); }
			}
			TWTSNoradConstPointer(const TWTSNoradPointer<_Ty>& src_cref) : TPointer<const TWTSNoradObj<_Ty>>(src_cref.m_ptr) {
				if (*this) { (*(*this)).increment_refcount(); }
			}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradConstPointer(const TWTSNoradPointer<_Ty2>& src_cref) : TPointer<const TWTSNoradObj<_Ty>>(src_cref.m_ptr) {
				if (*this) { (*(*this)).increment_refcount(); }
			}

			TWTSNoradConstPointer(TWTSNoradConstPointer&& src_ref) : TPointer<const TWTSNoradObj<_Ty>>(std::forward<decltype(src_ref.m_ptr)>(src_ref.m_ptr)) {
				src_ref.m_ptr = nullptr;
			}
			TWTSNoradConstPointer(TWTSNoradPointer<_Ty>&& src_ref) : TPointer<const TWTSNoradObj<_Ty>>(std::forward<decltype(src_ref.m_ptr)>(src_ref.m_ptr)) {
				src_ref.m_ptr = nullptr;
			}

			TWTSNoradConstPointer(std::nullptr_t) : TPointer<const TWTSNoradObj<_Ty>>(nullptr) {}
			virtual ~TWTSNoradConstPointer() {
				if (*this) { (*(*this)).decrement_refcount(); }
			}
			TWTSNoradConstPointer<_Ty>& operator=(const TWTSNoradConstPointer<_Ty>& _Right_cref) {
				if (*this) { (*(*this)).decrement_refcount(); }
				TPointer<const TWTSNoradObj<_Ty>>::operator=(_Right_cref);
				if (*this) { (*(*this)).increment_refcount(); }
				return (*this);
			}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradConstPointer<_Ty>& operator=(const TWTSNoradConstPointer<_Ty2>& _Right_cref) {
				return (*this).operator=(TWTSNoradConstPointer(_Right_cref));
			}

			const TWTSNoradObj<_Ty>& operator*() const {
				if (!((*this).m_ptr)) { MSE_THROW(primitives_null_dereference_error("attempt to dereference null pointer - mse::TTSNoradPointer")); }
				return *((*this).m_ptr);
			}
			const TWTSNoradObj<_Ty>* operator->() const {
				if (!((*this).m_ptr)) { MSE_THROW(primitives_null_dereference_error("attempt to dereference null pointer - mse::TTSNoradPointer")); }
				return (*this).m_ptr;
			}

			operator bool() const { return (*this).m_ptr; }
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			operator const _Ty*() const {
#ifdef NATIVE_PTR_DEBUG_HELPER1
				if (nullptr == (*this).m_ptr) {
					int q = 5; /* just a line of code for putting a debugger break point */
				}
#endif /*NATIVE_PTR_DEBUG_HELPER1*/
				return (*this).m_ptr;
			}

			/* In C++, if an object is deleted via a pointer to its base class and the base class' destructor is not virtual,
			then the (derived) object's destructor won't be called possibly resulting in resource leaks. With registered
			objects, the destructor not being called also circumvents their memory safety mechanism. */
			void norad_delete() {
				auto a = asANativePointerToTWTSNoradObj();
				(*this) = nullptr;
				delete a;
			}

		private:
			TWTSNoradConstPointer(const TWTSNoradObj<_Ty>* ptr) : TPointer<const TWTSNoradObj<_Ty>>(ptr) {
				assert(*this);
				(*(*this)).increment_refcount();
			}

			/* This function, if possible, should not be used. It is meant to be used exclusively by norad_delete<>(). */
			const TWTSNoradObj<_Ty>* asANativePointerToTWTSNoradObj() const {
#ifdef NATIVE_PTR_DEBUG_HELPER1
				if (nullptr == (*this).m_ptr) {
					int q = 5; /* just a line of code for putting a debugger break point */
				}
#endif /*NATIVE_PTR_DEBUG_HELPER1*/
				return static_cast<const TWTSNoradObj<_Ty>*>((*this).m_ptr);
			}

			MSE_DEFAULT_OPERATOR_AMPERSAND_DECLARATION;

			template <class Y> friend class TWTSNoradConstPointer;
			friend class TWTSNoradNotNullConstPointer<_Ty>;
		};

		template<typename _Ty>
		class TWTSNoradNotNullPointer : public TWTSNoradPointer<_Ty>, public NeverNullTagBase {
		public:
			TWTSNoradNotNullPointer(const TWTSNoradNotNullPointer& src_cref) : TWTSNoradPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradNotNullPointer(const TWTSNoradNotNullPointer<_Ty2>& src_cref) : TWTSNoradPointer<_Ty>(src_cref) {}
			TWTSNoradNotNullPointer(TWTSNoradNotNullPointer&& src_ref) : TWTSNoradPointer<_Ty>(std::forward<decltype(src_ref)>(src_ref)) {}

			virtual ~TWTSNoradNotNullPointer() {}
			/*
			TWTSNoradNotNullPointer<_Ty>& operator=(const TWTSNoradNotNullPointer<_Ty>& _Right_cref) {
			TWTSNoradPointer<_Ty>::operator=(_Right_cref);
			return (*this);
			}
			*/

			TWTSNoradObj<_Ty>& operator*() const {
				//if (!((*this).m_ptr)) { MSE_THROW(primitives_null_dereference_error("attempt to dereference null pointer - mse::TTSNoradPointer")); }
				return *((*this).m_ptr);
			}
			TWTSNoradObj<_Ty>* operator->() const {
				//if (!((*this).m_ptr)) { MSE_THROW(primitives_null_dereference_error("attempt to dereference null pointer - mse::TTSNoradPointer")); }
				return (*this).m_ptr;
			}

			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			explicit operator _Ty*() const { return TWTSNoradPointer<_Ty>::operator _Ty*(); }
			explicit operator TWTSNoradObj<_Ty>*() const { return TWTSNoradPointer<_Ty>::operator TWTSNoradObj<_Ty>*(); }

		private:
			TWTSNoradNotNullPointer(TWTSNoradObj<_Ty>* ptr) : TWTSNoradPointer<_Ty>(ptr) {}

			/* If you want to use this constructor, use not_null_from_nullable() instead. */
			TWTSNoradNotNullPointer(const  TWTSNoradPointer<_Ty>& src_cref) : TWTSNoradPointer<_Ty>(src_cref) {
				*src_cref; // to ensure that src_cref points to a valid target
			}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradNotNullPointer(const TWTSNoradPointer<_Ty2>& src_cref) : TWTSNoradPointer<_Ty>(src_cref) {
				*src_cref; // to ensure that src_cref points to a valid target
			}

			/* If you want a pointer to a TWTSNoradNotNullPointer<_Ty>, declare the TWTSNoradNotNullPointer<_Ty> as a
			TWTSNoradObj<TWTSNoradNotNullPointer<_Ty>> instead. So for example:
			auto reg_ptr = TWTSNoradObj<TWTSNoradNotNullPointer<_Ty>>(mse::registered_new<_Ty>());
			auto reg_ptr_to_reg_ptr = &reg_ptr;
			*/
			MSE_DEFAULT_OPERATOR_AMPERSAND_DECLARATION;

			friend class TWTSNoradFixedPointer<_Ty>;
			template<typename _Ty2>
			friend TWTSNoradNotNullPointer<_Ty2> not_null_from_nullable(const TWTSNoradPointer<_Ty2>& src);
		};

		template<typename _Ty>
		class TWTSNoradNotNullConstPointer : public TWTSNoradConstPointer<_Ty>, public NeverNullTagBase {
		public:
			TWTSNoradNotNullConstPointer(const TWTSNoradNotNullPointer<_Ty>& src_cref) : TWTSNoradConstPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradNotNullConstPointer(const TWTSNoradNotNullPointer<_Ty2>& src_cref) : TWTSNoradConstPointer<_Ty>(src_cref) {}
			TWTSNoradNotNullConstPointer(const TWTSNoradNotNullConstPointer<_Ty>& src_cref) : TWTSNoradConstPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradNotNullConstPointer(const TWTSNoradNotNullConstPointer<_Ty2>& src_cref) : TWTSNoradConstPointer<_Ty>(src_cref) {}

			TWTSNoradNotNullConstPointer(TWTSNoradNotNullPointer<_Ty>&& src_ref) : TWTSNoradConstPointer<_Ty>(std::forward<decltype(src_ref)>(src_ref)) {}
			TWTSNoradNotNullConstPointer(TWTSNoradNotNullConstPointer<_Ty>&& src_ref) : TWTSNoradConstPointer<_Ty>(std::forward<decltype(src_ref)>(src_ref)) {}

			virtual ~TWTSNoradNotNullConstPointer() {}

			const TWTSNoradObj<_Ty>& operator*() const {
				//if (!((*this).m_ptr)) { MSE_THROW(primitives_null_dereference_error("attempt to dereference null pointer - mse::TTSNoradPointer")); }
				return *((*this).m_ptr);
			}
			const TWTSNoradObj<_Ty>* operator->() const {
				//if (!((*this).m_ptr)) { MSE_THROW(primitives_null_dereference_error("attempt to dereference null pointer - mse::TTSNoradPointer")); }
				return (*this).m_ptr;
			}

			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			explicit operator const _Ty*() const { return TWTSNoradConstPointer<_Ty>::operator const _Ty*(); }
			explicit operator const TWTSNoradObj<_Ty>*() const { return TWTSNoradConstPointer<_Ty>::operator const TWTSNoradObj<_Ty>*(); }

		private:
			TWTSNoradNotNullConstPointer(const TWTSNoradObj<_Ty>* ptr) : TWTSNoradConstPointer<_Ty>(ptr) {}

			/* If you want to use this constructor, use not_null_from_nullable() instead. */
			TWTSNoradNotNullConstPointer(const TWTSNoradPointer<_Ty>& src_cref) : TWTSNoradConstPointer<_Ty>(src_cref) {
				*src_cref; // to ensure that src_cref points to a valid target
			}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradNotNullConstPointer(const TWTSNoradPointer<_Ty2>& src_cref) : TWTSNoradConstPointer<_Ty>(src_cref) {
				*src_cref; // to ensure that src_cref points to a valid target
			}
			TWTSNoradNotNullConstPointer(const TWTSNoradConstPointer<_Ty>& src_cref) : TWTSNoradConstPointer<_Ty>(src_cref) {
				*src_cref; // to ensure that src_cref points to a valid target
			}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradNotNullConstPointer(const TWTSNoradConstPointer<_Ty2>& src_cref) : TWTSNoradConstPointer<_Ty>(src_cref) {
				*src_cref; // to ensure that src_cref points to a valid target
			}

			MSE_DEFAULT_OPERATOR_AMPERSAND_DECLARATION;

			friend class TWTSNoradFixedConstPointer<_Ty>;
			template<typename _Ty2>
			friend TWTSNoradNotNullConstPointer<_Ty2> not_null_from_nullable(const TWTSNoradConstPointer<_Ty2>& src);
		};

		template<typename _Ty>
		TWTSNoradNotNullPointer<_Ty> not_null_from_nullable(const TWTSNoradPointer<_Ty>& src) {
			return src;
		}
		template<typename _Ty>
		TWTSNoradNotNullConstPointer<_Ty> not_null_from_nullable(const TWTSNoradConstPointer<_Ty>& src) {
			return src;
		}

		template<typename _Ty>
		class TWTSNoradFixedPointer : public TWTSNoradNotNullPointer<_Ty> {
		public:
			TWTSNoradFixedPointer(const TWTSNoradFixedPointer& src_cref) : TWTSNoradNotNullPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradFixedPointer(const TWTSNoradFixedPointer<_Ty2>& src_cref) : TWTSNoradNotNullPointer<_Ty>(src_cref) {}

			TWTSNoradFixedPointer(const TWTSNoradNotNullPointer<_Ty>& src_cref) : TWTSNoradNotNullPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradFixedPointer(const TWTSNoradNotNullPointer<_Ty2>& src_cref) : TWTSNoradNotNullPointer<_Ty>(src_cref) {}

			TWTSNoradFixedPointer(TWTSNoradFixedPointer&& src_ref) : TWTSNoradNotNullPointer<_Ty>(std::forward<decltype(src_ref)>(src_ref)) {}
			TWTSNoradFixedPointer(TWTSNoradNotNullPointer<_Ty>&& src_ref) : TWTSNoradNotNullPointer<_Ty>(std::forward<decltype(src_ref)>(src_ref)) {}

			virtual ~TWTSNoradFixedPointer() {}

			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			explicit operator _Ty*() const { return TWTSNoradNotNullPointer<_Ty>::operator _Ty*(); }
			explicit operator TWTSNoradObj<_Ty>*() const { return TWTSNoradNotNullPointer<_Ty>::operator TWTSNoradObj<_Ty>*(); }

		private:
			TWTSNoradFixedPointer(TWTSNoradObj<_Ty>* ptr) : TWTSNoradNotNullPointer<_Ty>(ptr) {}
			TWTSNoradFixedPointer<_Ty>& operator=(const TWTSNoradFixedPointer<_Ty>& _Right_cref) = delete;

			/* If you want a pointer to a TWTSNoradFixedPointer<_Ty>, declare the TWTSNoradFixedPointer<_Ty> as a
			TWTSNoradObj<TWTSNoradFixedPointer<_Ty>> instead. So for example:
			auto reg_ptr = TWTSNoradObj<TWTSNoradFixedPointer<_Ty>>(mse::registered_new<_Ty>());
			auto reg_ptr_to_reg_ptr = &reg_ptr;
			*/
			MSE_DEFAULT_OPERATOR_AMPERSAND_DECLARATION;

			friend class TWTSNoradObj<_Ty>;
		};

		template<typename _Ty>
		class TWTSNoradFixedConstPointer : public TWTSNoradNotNullConstPointer<_Ty> {
		public:
			TWTSNoradFixedConstPointer(const TWTSNoradFixedPointer<_Ty>& src_cref) : TWTSNoradNotNullConstPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradFixedConstPointer(const TWTSNoradFixedPointer<_Ty2>& src_cref) : TWTSNoradNotNullConstPointer<_Ty>(src_cref) {}
			TWTSNoradFixedConstPointer(const TWTSNoradFixedConstPointer<_Ty>& src_cref) : TWTSNoradNotNullConstPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradFixedConstPointer(const TWTSNoradFixedConstPointer<_Ty2>& src_cref) : TWTSNoradNotNullConstPointer<_Ty>(src_cref) {}

			TWTSNoradFixedConstPointer(const TWTSNoradNotNullPointer<_Ty>& src_cref) : TWTSNoradNotNullConstPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradFixedConstPointer(const TWTSNoradNotNullPointer<_Ty2>& src_cref) : TWTSNoradNotNullConstPointer<_Ty>(src_cref) {}
			TWTSNoradFixedConstPointer(const TWTSNoradNotNullConstPointer<_Ty>& src_cref) : TWTSNoradNotNullConstPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TWTSNoradFixedConstPointer(const TWTSNoradNotNullConstPointer<_Ty2>& src_cref) : TWTSNoradNotNullConstPointer<_Ty>(src_cref) {}

			TWTSNoradFixedConstPointer(TWTSNoradFixedPointer<_Ty>&& src_ref) : TWTSNoradNotNullConstPointer<_Ty>(std::forward<decltype(src_ref)>(src_ref)) {}
			TWTSNoradFixedConstPointer(TWTSNoradFixedConstPointer<_Ty>&& src_ref) : TWTSNoradNotNullConstPointer<_Ty>(std::forward<decltype(src_ref)>(src_ref)) {}

			TWTSNoradFixedConstPointer(TWTSNoradNotNullPointer<_Ty>&& src_ref) : TWTSNoradNotNullConstPointer<_Ty>(std::forward<decltype(src_ref)>(src_ref)) {}
			TWTSNoradFixedConstPointer(TWTSNoradNotNullConstPointer<_Ty>&& src_ref) : TWTSNoradNotNullConstPointer<_Ty>(std::forward<decltype(src_ref)>(src_ref)) {}

			virtual ~TWTSNoradFixedConstPointer() {}
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			explicit operator const _Ty*() const { return TWTSNoradNotNullConstPointer<_Ty>::operator const _Ty*(); }
			explicit operator const TWTSNoradObj<_Ty>*() const { return TWTSNoradNotNullConstPointer<_Ty>::operator const TWTSNoradObj<_Ty>*(); }

		private:
			TWTSNoradFixedConstPointer(const TWTSNoradObj<_Ty>* ptr) : TWTSNoradNotNullConstPointer<_Ty>(ptr) {}
			TWTSNoradFixedConstPointer<_Ty>& operator=(const TWTSNoradFixedConstPointer<_Ty>& _Right_cref) = delete;

			MSE_DEFAULT_OPERATOR_AMPERSAND_DECLARATION;

			friend class TWTSNoradObj<_Ty>;
		};

		template<typename _TROFLy>
		class TWTSNoradObj : public _TROFLy, public std::conditional<(!std::is_convertible<_TROFLy*, NotAsyncShareableTagBase*>::value) && (!std::is_base_of<NotAsyncShareableTagBase, _TROFLy>::value)
			, NotAsyncShareableTagBase, TPlaceHolder_msepointerbasics<TWTSNoradObj<_TROFLy> > >::type
		{
		public:
			typedef _TROFLy base_class;

			MSE_NORAD_OBJ_USING(TWTSNoradObj, _TROFLy);
			TWTSNoradObj(const TWTSNoradObj& _X) : _TROFLy(_X) {}
			TWTSNoradObj(TWTSNoradObj&& _X) : _TROFLy(std::forward<decltype(_X)>(_X)) {}
			virtual ~TWTSNoradObj() {
				std::lock_guard<decltype(m_destructor_mutex)> lock1(m_destructor_mutex);
				if (0 != m_counter) {
					/* It would be unsafe to allow this object to be destroyed as there are outstanding references to this object. */
					std::cerr << "\n\nFatal Error: mse::TWTSNoradObj<> destructed with outstanding references \n\n";
					std::terminate();
				}
			}

			template<class _Ty2>
			TWTSNoradObj& operator=(_Ty2&& _X) { _TROFLy::operator=(std::forward<decltype(_X)>(_X)); return (*this); }
			template<class _Ty2>
			TWTSNoradObj& operator=(const _Ty2& _X) { _TROFLy::operator=(_X); return (*this); }

			TWTSNoradFixedPointer<_TROFLy> operator&() {
				std::lock_guard<decltype(m_destructor_mutex)> lock1(m_destructor_mutex);
				return TWTSNoradFixedPointer<_TROFLy>(this);
			}
			TWTSNoradFixedConstPointer<_TROFLy> operator&() const {
				std::lock_guard<decltype(m_destructor_mutex)> lock1(m_destructor_mutex);
				return TWTSNoradFixedConstPointer<_TROFLy>(this);
			}
			TWTSNoradFixedPointer<_TROFLy> mse_norad_fptr() { std::lock_guard<decltype(m_destructor_mutex)> lock1(m_destructor_mutex); return TWTSNoradFixedPointer<_TROFLy>(this); }
			TWTSNoradFixedConstPointer<_TROFLy> mse_norad_fptr() const { std::lock_guard<decltype(m_destructor_mutex)> lock1(m_destructor_mutex); return TWTSNoradFixedConstPointer<_TROFLy>(this); }

			/* todo: make these private */
			void increment_refcount() const { m_counter += 1; }
			void decrement_refcount() const { m_counter -= 1; }

		private:
			mutable std::atomic<int> m_counter = 0;
			mutable std::mutex m_destructor_mutex;
		};

		template<typename _TROz> using TStaticImmutableObjBase = TWTSNoradObj<_TROz>;
		template<typename _Ty> using TStaticImmutablePointerBase = mse::us::impl::TAnyPointerBase<_Ty>;
		template<typename _Ty> using TStaticImmutableConstPointerBase = mse::us::impl::TAnyConstPointerBase<_Ty>;
		template<typename _Ty> using Tstatic_obj_base_ptr = TWTSNoradFixedPointer<_Ty>;
		template<typename _Ty> using Tstatic_obj_base_const_ptr = TWTSNoradConstPointer<_Ty>;

#else // MSE_STATICPOINTER_RUNTIME_CHECKS_ENABLED

		template<typename _TROz>
		class TStaticImmutableObjBase : public std::add_const<_TROz>::type {
		public:
			typedef const _TROz base_class;
			MSE_STATIC_USING(TStaticImmutableObjBase, base_class);
			TStaticImmutableObjBase(const TStaticImmutableObjBase& _X) : base_class(_X) {}
			TStaticImmutableObjBase(TStaticImmutableObjBase&& _X) : base_class(std::forward<decltype(_X)>(_X)) {}

			TStaticImmutableObjBase& operator=(TStaticImmutableObjBase&& _X) { base_class::operator=(std::forward<decltype(_X)>(_X)); return (*this); }
			TStaticImmutableObjBase& operator=(const TStaticImmutableObjBase& _X) { base_class::operator=(_X); return (*this); }
			template<class _Ty2>
			TStaticImmutableObjBase& operator=(_Ty2&& _X) { base_class::operator=(std::forward<decltype(_X)>(_X)); return (*this); }
			template<class _Ty2>
			TStaticImmutableObjBase& operator=(const _Ty2& _X) { base_class::operator=(_X); return (*this); }

			auto operator&() {
				return this;
			}
			auto operator&() const {
				return this;
			}
		};

		template<typename _Ty> using TStaticImmutablePointerBase = TPointerForLegacy<const _Ty, TStaticImmutableID<const _Ty>>;
		template<typename _Ty> using TStaticImmutableConstPointerBase = TPointerForLegacy<const _Ty, TStaticImmutableID<const _Ty>>;
		template<typename _Ty> using Tstatic_obj_base_ptr = TStaticImmutableObjBase<_Ty>*;
		template<typename _Ty> using Tstatic_obj_base_const_ptr = TStaticImmutableObjBase<const _Ty> const*;

#endif // MSE_STATICPOINTER_RUNTIME_CHECKS_ENABLED

		template <class _Ty, class _Ty2, class = typename std::enable_if<
			(!std::is_same<_Ty&&, _Ty2>::value) || (!std::is_rvalue_reference<_Ty2>::value)
			, void>::type>
			static void valid_if_not_rvalue_reference_of_given_type_msestatic(_Ty2 src) {}

		template<typename _Ty> class TStaticImmutableObj;
		template<typename _Ty> class TStaticImmutableNotNullPointer;
		template<typename _Ty> class TStaticImmutableNotNullConstPointer;
		template<typename _Ty> class TStaticImmutableFixedPointer;
		template<typename _Ty> class TStaticImmutableFixedConstPointer;
	}

	namespace us {
		namespace impl {
			template <typename _Ty, typename _TConstPointer1> class TCommonizedPointer;
			template <typename _Ty, typename _TConstPointer1> class TCommonizedConstPointer;
		}
	}

	namespace rsv {

		/* Use TStaticImmutableFixedPointer instead. */
		template<typename _Ty>
		class TStaticImmutablePointer : public TStaticImmutablePointerBase<_Ty>, public StrongPointerTagBase {
		public:
			typedef Tstatic_obj_base_ptr<_Ty> scope_obj_base_ptr_t;
			virtual ~TStaticImmutablePointer() {}
		private:
			TStaticImmutablePointer() : TStaticImmutablePointerBase<_Ty>() {}
			TStaticImmutablePointer(scope_obj_base_ptr_t ptr) : TStaticImmutablePointerBase<_Ty>(ptr) {}
			TStaticImmutablePointer(const TStaticImmutablePointer& src_cref) : TStaticImmutablePointerBase<_Ty>(
				static_cast<const TStaticImmutablePointerBase<_Ty>&>(src_cref)) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TStaticImmutablePointer(const TStaticImmutablePointer<_Ty2>& src_cref) : TStaticImmutablePointerBase<_Ty>(TStaticImmutablePointerBase<_Ty2>(src_cref)) {}
			TStaticImmutablePointer<_Ty>& operator=(TStaticImmutableObj<_Ty>* ptr) {
				return TStaticImmutablePointerBase<_Ty>::operator=(ptr);
			}
			TStaticImmutablePointer<_Ty>& operator=(const TStaticImmutablePointer<_Ty>& _Right_cref) {
				return TStaticImmutablePointerBase<_Ty>::operator=(_Right_cref);
			}
			operator bool() const {
				bool retval = (bool(*static_cast<const TStaticImmutablePointerBase<_Ty>*>(this)));
				return retval;
			}
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			explicit operator const _Ty*() const {
				const _Ty* retval = std::addressof(*(*this))/*(*static_cast<const TStaticImmutablePointerBase<_Ty>*>(this))*/;
				return retval;
			}
			explicit operator TStaticImmutableObj<_Ty>*() const {
				TStaticImmutableObj<_Ty>* retval = (*static_cast<const TStaticImmutablePointerBase<_Ty>*>(this));
				return retval;
			}

			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			friend class TStaticImmutableNotNullPointer<_Ty>;
			friend class mse::us::impl::TCommonizedPointer<_Ty, TStaticImmutablePointer<_Ty> >;
			friend class mse::us::impl::TCommonizedConstPointer<const _Ty, TStaticImmutablePointer<_Ty> >;
		};

		/* Use TStaticImmutableFixedConstPointer instead. */
		template<typename _Ty>
		class TStaticImmutableConstPointer : public TStaticImmutableConstPointerBase<const _Ty>, public StrongPointerTagBase {
		public:
			typedef Tstatic_obj_base_const_ptr<_Ty> scope_obj_base_const_ptr_t;
			virtual ~TStaticImmutableConstPointer() {}
		private:
			TStaticImmutableConstPointer() : TStaticImmutableConstPointerBase<const _Ty>() {}
			TStaticImmutableConstPointer(scope_obj_base_const_ptr_t ptr) : TStaticImmutableConstPointerBase<const _Ty>(ptr) {}
			TStaticImmutableConstPointer(const TStaticImmutableConstPointer& src_cref) : TStaticImmutableConstPointerBase<const _Ty>(static_cast<const TStaticImmutableConstPointerBase<const _Ty>&>(src_cref)) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TStaticImmutableConstPointer(const TStaticImmutableConstPointer<_Ty2>& src_cref) : TStaticImmutableConstPointerBase<const _Ty>(src_cref) {}
			TStaticImmutableConstPointer(const TStaticImmutablePointer<_Ty>& src_cref) : TStaticImmutableConstPointerBase<const _Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TStaticImmutableConstPointer(const TStaticImmutablePointer<_Ty2>& src_cref) : TStaticImmutableConstPointerBase<const _Ty>(TStaticImmutableConstPointerBase<_Ty2>(src_cref)) {}
			TStaticImmutableConstPointer<_Ty>& operator=(const TStaticImmutableObj<_Ty>* ptr) {
				return TStaticImmutableConstPointerBase<_Ty>::operator=(ptr);
			}
			TStaticImmutableConstPointer<_Ty>& operator=(const TStaticImmutableConstPointer<_Ty>& _Right_cref) {
				return TStaticImmutableConstPointerBase<_Ty>::operator=(_Right_cref);
			}
			operator bool() const {
				bool retval = (bool(*static_cast<const TStaticImmutableConstPointerBase<const _Ty>*>(this)));
				return retval;
			}
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			explicit operator const _Ty*() const {
				const _Ty* retval = (*static_cast<const TStaticImmutableConstPointerBase<const _Ty>*>(this));
				return retval;
			}
			explicit operator const TStaticImmutableObj<_Ty>*() const {
				const TStaticImmutableObj<_Ty>* retval = (*static_cast<const TStaticImmutableConstPointerBase<const _Ty>*>(this));
				return retval;
			}

			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			friend class TStaticImmutableNotNullConstPointer<_Ty>;
			friend class mse::us::impl::TCommonizedConstPointer<const _Ty, TStaticImmutableConstPointer<_Ty> >;
		};

		/* Use TStaticImmutableFixedPointer instead. */
		template<typename _Ty>
		class TStaticImmutableNotNullPointer : public TStaticImmutablePointer<_Ty>, public NeverNullTagBase {
		public:
			virtual ~TStaticImmutableNotNullPointer() {}
		private:
			TStaticImmutableNotNullPointer(typename TStaticImmutablePointer<_Ty>::scope_obj_base_ptr_t src_cref) : TStaticImmutablePointer<_Ty>(src_cref) {}
			TStaticImmutableNotNullPointer(TStaticImmutableObj<_Ty>* ptr) : TStaticImmutablePointer<_Ty>(ptr) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TStaticImmutableNotNullPointer(const TStaticImmutableNotNullPointer<_Ty2>& src_cref) : TStaticImmutablePointer<_Ty>(src_cref) {}
			TStaticImmutableNotNullPointer<_Ty>& operator=(const TStaticImmutablePointer<_Ty>& _Right_cref) {
				TStaticImmutablePointer<_Ty>::operator=(_Right_cref);
				return (*this);
			}
			operator bool() const { return (*static_cast<const TStaticImmutablePointer<_Ty>*>(this)); }
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			explicit operator const _Ty*() const { return TStaticImmutablePointer<_Ty>::operator const _Ty*(); }
			explicit operator TStaticImmutableObj<_Ty>*() const { return TStaticImmutablePointer<_Ty>::operator TStaticImmutableObj<_Ty>*(); }

			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			friend class TStaticImmutableFixedPointer<_Ty>;
		};

		/* Use TStaticImmutableFixedConstPointer instead. */
		template<typename _Ty>
		class TStaticImmutableNotNullConstPointer : public TStaticImmutableConstPointer<_Ty>, public NeverNullTagBase {
		public:
			virtual ~TStaticImmutableNotNullConstPointer() {}
		private:
			TStaticImmutableNotNullConstPointer(const TStaticImmutableNotNullConstPointer<_Ty>& src_cref) : TStaticImmutableConstPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TStaticImmutableNotNullConstPointer(const TStaticImmutableNotNullConstPointer<_Ty2>& src_cref) : TStaticImmutableConstPointer<_Ty>(src_cref) {}
			TStaticImmutableNotNullConstPointer(const TStaticImmutableNotNullPointer<_Ty>& src_cref) : TStaticImmutableConstPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TStaticImmutableNotNullConstPointer(const TStaticImmutableNotNullPointer<_Ty2>& src_cref) : TStaticImmutableConstPointer<_Ty>(src_cref) {}
			operator bool() const { return (*static_cast<const TStaticImmutableConstPointer<_Ty>*>(this)); }
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			explicit operator const _Ty*() const { return TStaticImmutableConstPointer<_Ty>::operator const _Ty*(); }
			explicit operator const TStaticImmutableObj<_Ty>*() const { return TStaticImmutableConstPointer<_Ty>::operator const TStaticImmutableObj<_Ty>*(); }
			TStaticImmutableNotNullConstPointer(typename TStaticImmutableConstPointer<_Ty>::scope_obj_base_const_ptr_t ptr) : TStaticImmutableConstPointer<_Ty>(ptr) {}

			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			friend class TStaticImmutableFixedConstPointer<_Ty>;
		};

		template<typename _Ty>
		class TStaticImmutableFixedPointer : public TStaticImmutableNotNullPointer<_Ty> {
		public:
			TStaticImmutableFixedPointer(const TStaticImmutableFixedPointer& src_cref) : TStaticImmutableNotNullPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TStaticImmutableFixedPointer(const TStaticImmutableFixedPointer<_Ty2>& src_cref) : TStaticImmutableNotNullPointer<_Ty>(src_cref) {}
			virtual ~TStaticImmutableFixedPointer() {}
			operator bool() const { return (*static_cast<const TStaticImmutableNotNullPointer<_Ty>*>(this)); }
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			explicit operator const _Ty*() const { return TStaticImmutableNotNullPointer<_Ty>::operator const _Ty*(); }
			explicit operator TStaticImmutableObj<_Ty>*() const { return TStaticImmutableNotNullPointer<_Ty>::operator TStaticImmutableObj<_Ty>*(); }
			void static_tag() const {}

		private:
			TStaticImmutableFixedPointer(typename TStaticImmutablePointer<_Ty>::scope_obj_base_ptr_t ptr) : TStaticImmutableNotNullPointer<_Ty>(ptr) {}
#ifdef MSE_STATIC_DISABLE_MOVE_RESTRICTIONS
			TStaticImmutableFixedPointer(TStaticImmutableFixedPointer&& src_ref) : TStaticImmutableNotNullPointer<_Ty>(src_ref) {
				int q = 5;
		}
#endif // !MSE_STATIC_DISABLE_MOVE_RESTRICTIONS
			TStaticImmutableFixedPointer<_Ty>& operator=(const TStaticImmutableFixedPointer<_Ty>& _Right_cref) = delete;
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			friend class TStaticImmutableObj<_Ty>;
	};

		template<typename _Ty>
		class TStaticImmutableFixedConstPointer : public TStaticImmutableNotNullConstPointer<_Ty> {
		public:
			TStaticImmutableFixedConstPointer(const TStaticImmutableFixedConstPointer<_Ty>& src_cref) : TStaticImmutableNotNullConstPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TStaticImmutableFixedConstPointer(const TStaticImmutableFixedConstPointer<_Ty2>& src_cref) : TStaticImmutableNotNullConstPointer<_Ty>(src_cref) {}
			TStaticImmutableFixedConstPointer(const TStaticImmutableFixedPointer<_Ty>& src_cref) : TStaticImmutableNotNullConstPointer<_Ty>(src_cref) {}
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TStaticImmutableFixedConstPointer(const TStaticImmutableFixedPointer<_Ty2>& src_cref) : TStaticImmutableNotNullConstPointer<_Ty>(src_cref) {}
			virtual ~TStaticImmutableFixedConstPointer() {}
			operator bool() const { return (*static_cast<const TStaticImmutableNotNullConstPointer<_Ty>*>(this)); }
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			explicit operator const _Ty*() const { return TStaticImmutableNotNullConstPointer<_Ty>::operator const _Ty*(); }
			explicit operator const TStaticImmutableObj<_Ty>*() const { return TStaticImmutableNotNullConstPointer<_Ty>::operator const TStaticImmutableObj<_Ty>*(); }
			void static_tag() const {}

		private:
			TStaticImmutableFixedConstPointer(typename TStaticImmutableConstPointer<_Ty>::scope_obj_base_const_ptr_t ptr) : TStaticImmutableNotNullConstPointer<_Ty>(ptr) {}
			TStaticImmutableFixedConstPointer<_Ty>& operator=(const TStaticImmutableFixedConstPointer<_Ty>& _Right_cref) = delete;
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			friend class TStaticImmutableObj<_Ty>;
		};

		template<typename _TROy>
		class TStaticImmutableObj : public TStaticImmutableObjBase<_TROy> {
		public:
			TStaticImmutableObj(const TStaticImmutableObj& _X) : TStaticImmutableObjBase<_TROy>(_X) {}

#ifdef MSE_STATIC_DISABLE_MOVE_RESTRICTIONS
			explicit TStaticImmutableObj(TStaticImmutableObj&& _X) : TStaticImmutableObjBase<_TROy>(std::forward<decltype(_X)>(_X)) {}
#endif // !MSE_STATIC_DISABLE_MOVE_RESTRICTIONS

			MSE_STATIC_USING(TStaticImmutableObj, TStaticImmutableObjBase<_TROy>);
			virtual ~TStaticImmutableObj() {
				mse::T_valid_if_is_marked_as_xscope_shareable_msemsearray<_TROy>();
			}

			TStaticImmutableObj& operator=(TStaticImmutableObj&& _X) {
				valid_if_not_rvalue_reference_of_given_type_msestatic<TStaticImmutableObj, decltype(_X)>(_X);
				TStaticImmutableObjBase<_TROy>::operator=(std::forward<decltype(_X)>(_X));
				return (*this);
			}
			TStaticImmutableObj& operator=(const TStaticImmutableObj& _X) { TStaticImmutableObjBase<_TROy>::operator=(_X); return (*this); }
			template<class _Ty2>
			TStaticImmutableObj& operator=(_Ty2&& _X) {
				TStaticImmutableObjBase<_TROy>::operator=(std::forward<decltype(_X)>(_X));
				return (*this);
			}
			template<class _Ty2>
			TStaticImmutableObj& operator=(const _Ty2& _X) { TStaticImmutableObjBase<_TROy>::operator=(_X); return (*this); }

			const TStaticImmutableFixedPointer<_TROy> operator&() & {
				return &(*static_cast<TStaticImmutableObjBase<_TROy>*>(this));
			}
			const TStaticImmutableFixedConstPointer<_TROy> operator&() const & {
				return &(*static_cast<const TStaticImmutableObjBase<_TROy>*>(this));
			}
			const TStaticImmutableFixedPointer<_TROy> mse_static_fptr() & { return &(*this); }
			const TStaticImmutableFixedConstPointer<_TROy> mse_static_fptr() const & { return &(*this); }

			void operator&() && = delete;
			void operator&() const && = delete;
			void mse_static_fptr() && = delete;
			void mse_static_fptr() const && = delete;

			void static_tag() const {}

		private:
			void* operator new(size_t size) { return ::operator new(size); }
		};

		template<typename _Ty>
		auto static_fptr_to(_Ty&& _X) {
			return _X.mse_static_fptr();
		}
		template<typename _Ty>
		auto static_fptr_to(const _Ty& _X) {
			return _X.mse_static_fptr();
		}
	}
}

namespace std {
	template<class _Ty>
	struct hash<mse::rsv::TStaticImmutableFixedPointer<_Ty> > {	// hash functor
		typedef mse::rsv::TStaticImmutableFixedPointer<_Ty> argument_type;
		typedef size_t result_type;
		size_t operator()(const mse::rsv::TStaticImmutableFixedPointer<_Ty>& _Keyval) const _NOEXCEPT {
			const _Ty* ptr1 = nullptr;
			if (_Keyval) {
				ptr1 = std::addressof(*_Keyval);
			}
			return (hash<const _Ty *>()(ptr1));
		}
	};

	template<class _Ty>
	struct hash<mse::rsv::TStaticImmutableFixedConstPointer<_Ty> > {	// hash functor
		typedef mse::rsv::TStaticImmutableFixedConstPointer<_Ty> argument_type;
		typedef size_t result_type;
		size_t operator()(const mse::rsv::TStaticImmutableFixedConstPointer<_Ty>& _Keyval) const _NOEXCEPT {
			const _Ty* ptr1 = nullptr;
			if (_Keyval) {
				ptr1 = std::addressof(*_Keyval);
			}
			return (hash<const _Ty *>()(ptr1));
		}
	};
}

namespace mse {
	namespace rsv {

		/* template specializations */

		template<typename _Ty>
		class TStaticImmutableObj<_Ty*> : public TStaticImmutableObj<mse::TPointerForLegacy<_Ty>> {
		public:
			typedef TStaticImmutableObj<mse::TPointerForLegacy<_Ty>> base_class;
			MSE_USING(TStaticImmutableObj, base_class);
		};
		template<typename _Ty>
		class TStaticImmutableObj<_Ty* const> : public TStaticImmutableObj<const mse::TPointerForLegacy<_Ty>> {
		public:
			typedef TStaticImmutableObj<const mse::TPointerForLegacy<_Ty>> base_class;
			MSE_USING(TStaticImmutableObj, base_class);
		};
		template<typename _Ty>
		class TStaticImmutableObj<const _Ty *> : public TStaticImmutableObj<mse::TPointerForLegacy<const _Ty>> {
		public:
			typedef TStaticImmutableObj<mse::TPointerForLegacy<const _Ty>> base_class;
			MSE_USING(TStaticImmutableObj, base_class);
		};
		template<typename _Ty>
		class TStaticImmutableObj<const _Ty * const> : public TStaticImmutableObj<const mse::TPointerForLegacy<const _Ty>> {
		public:
			typedef TStaticImmutableObj<const mse::TPointerForLegacy<const _Ty>> base_class;
			MSE_USING(TStaticImmutableObj, base_class);
		};

#ifdef MSEPRIMITIVES_H
		template<>
		class TStaticImmutableObj<int> : public TStaticImmutableObj<mse::TInt<int>> {
		public:
			typedef TStaticImmutableObj<mse::TInt<int>> base_class;
			MSE_USING(TStaticImmutableObj, base_class);
		};
		template<>
		class TStaticImmutableObj<const int> : public TStaticImmutableObj<const mse::TInt<int>> {
		public:
			typedef TStaticImmutableObj<const mse::TInt<int>> base_class;
			MSE_USING(TStaticImmutableObj, base_class);
		};

		template<>
		class TStaticImmutableObj<size_t> : public TStaticImmutableObj<mse::TInt<size_t>> {
		public:
			typedef TStaticImmutableObj<mse::TInt<size_t>> base_class;
			MSE_USING(TStaticImmutableObj, base_class);
		};
		template<>
		class TStaticImmutableObj<const size_t> : public TStaticImmutableObj<const mse::TInt<size_t>> {
		public:
			typedef TStaticImmutableObj<const mse::TInt<size_t>> base_class;
			MSE_USING(TStaticImmutableObj, base_class);
		};
#endif /*MSEPRIMITIVES_H*/

		/* end of template specializations */

#endif /*MSE_STATICPOINTER_DISABLED*/
	}

#define MSE_RSV_DECLARE_STATIC_IMMUTABLE(type) static mse::rsv::TStaticImmutableObj<type> 
#define MSE_RSV_DECLARE_GLOBAL_IMMUTABLE(type) mse::rsv::TStaticImmutableObj<type> 


	class CStaticImmutablePtrTest1 {
	public:
		static void s_test1() {
#ifdef MSE_SELF_TESTS
			class A {
			public:
				A(int x) : b(x) {}
				A(const A& _X) : b(_X.b) {}
				A(A&& _X) : b(std::forward<decltype(_X.b)>(_X.b)) {}
				virtual ~A() {}
				A& operator=(A&& _X) { b = std::forward<decltype(_X.b)>(_X.b); return (*this); }
				A& operator=(const A& _X) { b = _X.b; return (*this); }

				int b = 3;
			};
			typedef mse::rsv::TAsyncShareableObj<A> shareable_A;
			class B {
			public:
				static int foo1(const shareable_A* a_native_ptr) { return a_native_ptr->b; }
				static int foo2(mse::rsv::TStaticImmutableFixedPointer<shareable_A> shareable_A_static_ptr) { return shareable_A_static_ptr->b; }
			protected:
				~B() {}
			};

			shareable_A* shareable_A_native_ptr = nullptr;

			{
				shareable_A a(7);
				MSE_RSV_DECLARE_STATIC_IMMUTABLE(shareable_A) static_a(7);

				assert(a.b == static_a.b);
				shareable_A_native_ptr = std::addressof(a);

				mse::rsv::TStaticImmutableFixedPointer<shareable_A> shareable_A_static_ptr1(&static_a);
				assert(shareable_A_native_ptr->b == shareable_A_static_ptr1->b);
				mse::rsv::TStaticImmutableFixedPointer<shareable_A> shareable_A_static_ptr2 = &static_a;

				/* mse::rsv::TStaticImmutableFixedPointers can be coerced into native pointers if you need to interact with legacy code or libraries. */
				B::foo1(static_cast<const shareable_A*>(shareable_A_static_ptr1));

				if (!shareable_A_static_ptr2) {
					assert(false);
				}
				else if (!(shareable_A_static_ptr2 != shareable_A_static_ptr1)) {
					int q = B::foo2(shareable_A_static_ptr2);
				}
				else {
					assert(false);
				}

				shareable_A a2 = a;
				MSE_RSV_DECLARE_STATIC_IMMUTABLE(shareable_A) static_a2 = static_a;
				static_a2 = a;
				static_a2 = static_a;

				mse::rsv::TStaticImmutableFixedConstPointer<shareable_A> rcp = shareable_A_static_ptr1;
				mse::rsv::TStaticImmutableFixedConstPointer<shareable_A> rcp2 = rcp;
			}

			{
				/* Polymorphic conversions. */
				class E {
				public:
					int m_b = 5;
				};

				/* Polymorphic conversions that would not be supported by mse::TRegisteredPointer. */
				class GE : public E {};
				typedef mse::rsv::TAsyncShareableObj<GE> shareable_GE;
				MSE_RSV_DECLARE_STATIC_IMMUTABLE(shareable_GE) static_gd;
				mse::rsv::TStaticImmutableFixedPointer<shareable_GE> GE_static_ifptr1 = &static_gd;
				mse::rsv::TStaticImmutableFixedPointer<E> E_static_ifptr5 = GE_static_ifptr1;
				mse::rsv::TStaticImmutableFixedPointer<E> E_static_fptr2(&static_gd);
				mse::rsv::TStaticImmutableFixedPointer<E> E_static_ifptr2(&static_gd);
				mse::rsv::TStaticImmutableFixedConstPointer<E> E_static_fcptr2 = &static_gd;
			}

			{
				class A {
				public:
					A(int x) : b(x) {}
					virtual ~A() {}

					int b = 3;
					std::string s = "some text ";
				};
				typedef mse::rsv::TAsyncShareableObj<A> shareable_A;
				class B {
				public:
					static int foo1(shareable_A* a_native_ptr) { return a_native_ptr->b; }
					static int foo2(mse::rsv::TStaticImmutableFixedPointer<shareable_A> shareable_A_scpfptr) { return shareable_A_scpfptr->b; }
					static int foo3(mse::rsv::TStaticImmutableFixedConstPointer<shareable_A> shareable_A_scpfcptr) { return shareable_A_scpfcptr->b; }
				protected:
					~B() {}
				};

				MSE_RSV_DECLARE_STATIC_IMMUTABLE(shareable_A) a_scpobj(5);
				int res1 = (&a_scpobj)->b;
				int res2 = B::foo2(&a_scpobj);
				int res3 = B::foo3(&a_scpobj);

				/* You can use the "mse::make_pointer_to_member_v2()" function to obtain a safe pointer to a member of
				an static object. */
				auto s_safe_ptr1 = mse::make_pointer_to_member_v2((&a_scpobj), &shareable_A::s);
				auto s_safe_const_ptr1 = mse::make_const_pointer_to_member_v2((&a_scpobj), &shareable_A::s);
			}

			{
				shareable_A a(7);
				MSE_RSV_DECLARE_STATIC_IMMUTABLE(shareable_A) static_a(7);

				assert(a.b == static_a.b);
				shareable_A_native_ptr = std::addressof(a);

				mse::rsv::TStaticImmutableFixedPointer<shareable_A> shareable_A_static_ptr1 = &static_a;
				assert(shareable_A_native_ptr->b == shareable_A_static_ptr1->b);
				mse::rsv::TStaticImmutableFixedPointer<shareable_A> shareable_A_static_ptr2 = &static_a;

				/* mse::rsv::TStaticImmutableFixedPointers can be coerced into native pointers if you need to interact with legacy code or libraries. */
				B::foo1(static_cast<const shareable_A*>(shareable_A_static_ptr1));

				if (!shareable_A_static_ptr2) {
					assert(false);
				}
				else if (!(shareable_A_static_ptr2 != shareable_A_static_ptr1)) {
					int q = B::foo2(shareable_A_static_ptr2);
				}
				else {
					assert(false);
				}

				shareable_A a2 = a;
				MSE_RSV_DECLARE_STATIC_IMMUTABLE(shareable_A) static_a2 = static_a;
				static_a2 = a;
				static_a2 = static_a;

				mse::rsv::TStaticImmutableFixedConstPointer<shareable_A> rcp = shareable_A_static_ptr1;
				mse::rsv::TStaticImmutableFixedConstPointer<shareable_A> rcp2 = rcp;
			}

			{
				/* Polymorphic conversions. */
				class E {
				public:
					int m_b = 5;
				};

				/* Polymorphic conversions that would not be supported by mse::TRegisteredPointer. */
				class GE : public E {};
				typedef mse::rsv::TAsyncShareableObj<GE> shareable_GE;
				MSE_RSV_DECLARE_STATIC_IMMUTABLE(shareable_GE) static_gd;
				mse::rsv::TStaticImmutableFixedPointer<shareable_GE> GE_static_ifptr1 = &static_gd;
				mse::rsv::TStaticImmutableFixedPointer<E> E_static_ptr5(GE_static_ifptr1);
				mse::rsv::TStaticImmutableFixedPointer<E> E_static_ifptr2(&static_gd);
				mse::rsv::TStaticImmutableFixedConstPointer<E> E_static_fcptr2 = &static_gd;
			}

#endif // MSE_SELF_TESTS
		}
	};

#ifdef __clang__
#pragma clang diagnostic pop
#else /*__clang__*/
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif /*__GNUC__*/
#endif /*__clang__*/

}

#ifdef _MSC_VER
#pragma warning( pop )  
#endif /*_MSC_VER*/

#endif // MSESTATIC_H_
