
// Copyright (c) 2015 Noah Lopez
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef MSESCOPE_TAGGED_H_
#define MSESCOPE_TAGGED_H_

#include "msescope.h"

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

/* Note that by default, MSE_SCOPEPOINTER_DISABLED is defined in non-debug builds. This is enacted in "msepointerbasics.h". */

#ifndef MSE_PUSH_MACRO_NOT_SUPPORTED
#pragma push_macro("_NOEXCEPT")
#endif // !MSE_PUSH_MACRO_NOT_SUPPORTED

#ifndef _NOEXCEPT
#define _NOEXCEPT
#endif /*_NOEXCEPT*/


namespace mse {

#define MSE_NEWTAG __LINE__

	/* This macro roughly simulates constructor inheritance. */
#define MSE_SCOPE_TAGGED_USING(Derived, Base) MSE_USING(Derived, Base)

	namespace impl {
		template<typename _Ty> class TScopeTaggedID {};
	}

	namespace us {
		namespace impl {
			class XScopeTaggedTagBase { public: void xscope_tagged_tag() const {} };

			/* Note that objects not derived from ReferenceableByScopeTaggedPointerTagBase might still be targeted by a scope pointer via
			make_pointer_to_member_v2(). */
			class ReferenceableByScopeTaggedPointerTagBase {};

			class ContainsNonOwningScopeTaggedReferenceTagBase {};
			class XScopeTaggedContainsNonOwningScopeTaggedReferenceTagBase : public ContainsNonOwningScopeTaggedReferenceTagBase, public XScopeTaggedTagBase {};
		}
	}

	namespace impl {

		template<typename T>
		struct HasXScopeTaggedReturnableTagMethod
		{
			template<typename U, void(U::*)() const> struct SFINAE {};
			template<typename U> static char Test(SFINAE<U, &U::xscope_tagged_returnable_tag>*);
			template<typename U> static int Test(...);
			static const bool Has = (sizeof(Test<T>(0)) == sizeof(char));
		};

		/*
		template<typename T>
		struct HasXScopeTaggedNotReturnableTagMethod
		{
			template<typename U, void(U::*)() const> struct SFINAE {};
			template<typename U> static char Test(SFINAE<U, &U::xscope_tagged_not_returnable_tag>*);
			template<typename U> static int Test(...);
			static const bool Has = (sizeof(Test<T>(0)) == sizeof(char));
		};
		*/

		template<typename T>
		struct is_nonowning_scope_tagged_pointer : std::integral_constant<bool, ((std::is_base_of<mse::us::impl::XScopeTaggedContainsNonOwningScopeTaggedReferenceTagBase, T>::value
			&& std::is_base_of<mse::us::impl::StrongPointerAsyncNotShareableAndNotPassableTagBase, T>::value)
			|| (std::is_pointer<T>::value && (!mse::impl::is_potentially_not_xscope<T>::value)))> {};
	}

#ifdef MSE_SCOPEPOINTER_DISABLED
	namespace rsv {
		template<typename _Ty, size_t TagID> using TXScopeTaggedObjPointer = _Ty*;
		template<typename _Ty, size_t TagID> using TXScopeTaggedObjConstPointer = const _Ty*;
		template<typename _Ty, size_t TagID> using TXScopeTaggedObjNotNullPointer = _Ty*;
		template<typename _Ty, size_t TagID> using TXScopeTaggedObjNotNullConstPointer = const _Ty*;
		template<typename _Ty, size_t TagID> using TXScopeTaggedObjFixedPointer = _Ty* /*const*/; /* Can't be const qualified because standard
																		   library containers don't support const elements. */
		template<typename _Ty, size_t TagID> using TXScopeTaggedObjFixedConstPointer = const _Ty* /*const*/;

		template<typename _TROy, size_t TagID> using TXScopeTaggedObj = _TROy;
		template<typename _Ty, size_t TagID> using TXScopeTaggedFixedPointer = _Ty* /*const*/;
		template<typename _Ty, size_t TagID> using TXScopeTaggedFixedConstPointer = const _Ty* /*const*/;
		template<typename _Ty, size_t TagID> using TXScopeTaggedCagedItemFixedPointerToRValue = _Ty* /*const*/;
		template<typename _Ty, size_t TagID> using TXScopeTaggedCagedItemFixedConstPointerToRValue = const _Ty* /*const*/;
		//template<typename _TROy, size_t TagID> using TXScopeTaggedReturnValue = _TROy;

		template<typename _TROy, size_t TagID> class TXScopeTaggedOwnerPointer;

		template<typename _Ty> auto xscope_tagged_ifptr_to(_Ty&& _X) { return std::addressof(_X); }
		template<typename _Ty> auto xscope_tagged_ifptr_to(const _Ty& _X) { return std::addressof(_X); }

		namespace us {
			template<size_t TagID, typename _Ty>
			TXScopeTaggedFixedPointer<_Ty, TagID> unsafe_make_xscope_tagged_pointer_to(_Ty& ref);
			template<size_t TagID, typename _Ty>
			TXScopeTaggedFixedConstPointer<_Ty, TagID> unsafe_make_xscope_tagged_const_pointer_to(const _Ty& cref);
		}

		//template<typename _Ty> const _Ty& return_value(const _Ty& _X) { return _X; }
		//template<typename _Ty> _Ty&& return_value(_Ty&& _X) { return std::forward<decltype(_X)>(_X); }
		template<typename _TROy> using TNonXScopeTaggedObj = _TROy;
	}

#else /*MSE_SCOPEPOINTER_DISABLED*/

	namespace rsv {
		template<typename _Ty, size_t ID/* = 0*/> class TXScopeTaggedObj;

		namespace us {
			namespace impl {

				template<typename _Ty, size_t TagID>
				class TXScopeTaggedObjPointerBase : public mse::us::impl::TPointerForLegacy<TXScopeObj<_Ty>, mse::impl::TScopeTaggedID<const _Ty>> {
				public:
					typedef mse::us::impl::TPointerForLegacy<TXScopeObj<_Ty>, mse::impl::TScopeTaggedID<const _Ty>> base_class;
					TXScopeTaggedObjPointerBase(const TXScopeTaggedObjPointerBase&) = default;
					TXScopeTaggedObjPointerBase(TXScopeTaggedObjPointerBase&&) = default;
					TXScopeTaggedObjPointerBase(TXScopeTaggedObj<_Ty, TagID>& scpobj_ref) : base_class(std::addressof(static_cast<TXScopeObj<_Ty>&>(scpobj_ref))) {}

					TXScopeTaggedObj<_Ty, TagID>& operator*() const {
						return static_cast<TXScopeTaggedObj<_Ty, TagID>&>(*(static_cast<const base_class&>(*this)));
					}
					TXScopeTaggedObj<_Ty, TagID>* operator->() const {
						return std::addressof(static_cast<TXScopeTaggedObj<_Ty, TagID>&>(*(static_cast<const base_class&>(*this))));
					}
				};

				template<typename _Ty, size_t TagID>
				class TXScopeTaggedObjConstPointerBase : public mse::us::impl::TPointerForLegacy<const TXScopeObj<_Ty>, mse::impl::TScopeTaggedID<const _Ty>> {
				public:
					typedef mse::us::impl::TPointerForLegacy<const TXScopeObj<_Ty>, mse::impl::TScopeTaggedID<const _Ty>> base_class;
					TXScopeTaggedObjConstPointerBase(const TXScopeTaggedObjConstPointerBase&) = default;
					TXScopeTaggedObjConstPointerBase(TXScopeTaggedObjConstPointerBase&&) = default;
					TXScopeTaggedObjConstPointerBase(const TXScopeTaggedObjPointerBase<_Ty, TagID>& src_cref) : base_class(src_cref) {}
					TXScopeTaggedObjConstPointerBase(const TXScopeTaggedObj<_Ty, TagID>& scpobj_cref) : base_class(std::addressof(static_cast<const TXScopeObj<_Ty>&>(scpobj_cref))) {}

					const TXScopeTaggedObj<_Ty, TagID>& operator*() const {
						return static_cast<const TXScopeTaggedObj<_Ty, TagID>&>(*(static_cast<const base_class&>(*this)));
					}
					const TXScopeTaggedObj<_Ty, TagID>* operator->() const {
						return std::addressof(static_cast<const TXScopeTaggedObj<_Ty, TagID>&>(*(static_cast<const base_class&>(*this))));
					}
				};

				template<typename _Ty, size_t TagID> using TXScopeTaggedItemPointerBase = mse::us::impl::TPointerForLegacy<_Ty, mse::impl::TScopeTaggedID<const _Ty>>;
				template<typename _Ty, size_t TagID> using TXScopeTaggedItemConstPointerBase = mse::us::impl::TPointerForLegacy<const _Ty, mse::impl::TScopeTaggedID<const _Ty>>;
			}
		}

		template<typename _Ty, size_t TagID> class TXScopeTaggedObj;
		template<typename _Ty, size_t TagID> class TXScopeTaggedObjNotNullPointer;
		template<typename _Ty, size_t TagID> class TXScopeTaggedObjNotNullConstPointer;
		template<typename _Ty, size_t TagID> class TXScopeTaggedObjFixedPointer;
		template<typename _Ty, size_t TagID> class TXScopeTaggedObjFixedConstPointer;
		template<typename _Ty, size_t TagID> class TXScopeTaggedOwnerPointer;

		template<typename _Ty, size_t TagID> class TXScopeTaggedFixedPointer;
		template<typename _Ty, size_t TagID> class TXScopeTaggedFixedConstPointer;
		template<typename _Ty, size_t TagID> class TXScopeTaggedCagedItemFixedPointerToRValue;
		template<typename _Ty, size_t TagID> class TXScopeTaggedCagedItemFixedConstPointerToRValue;

		template<typename _Ty, size_t TagID> class TXScopeTaggedFixedPointerFParam;
		template<typename _Ty, size_t TagID> class TXScopeTaggedFixedConstPointerFParam;

		namespace us {
			namespace impl {
				template <typename _Ty, typename _TConstPointer1> class TCommonizedPointer;
				template <typename _Ty, typename _TConstPointer1> class TCommonizedConstPointer;
			}
		}

		/* Use TXScopeTaggedObjFixedPointer instead. */
		template<typename _Ty, size_t TagID>
		class TXScopeTaggedObjPointer : public mse::rsv::us::impl::TXScopeTaggedObjPointerBase<_Ty, TagID>, public mse::us::impl::XScopeTaggedContainsNonOwningScopeTaggedReferenceTagBase, public mse::us::impl::StrongPointerAsyncNotShareableAndNotPassableTagBase {
		public:
			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedObjPointer() {}
		private:
			TXScopeTaggedObjPointer() : mse::rsv::us::impl::TXScopeTaggedObjPointerBase<_Ty, TagID>() {}
			TXScopeTaggedObjPointer(TXScopeTaggedObj<_Ty, TagID>& scpobj_ref) : mse::rsv::us::impl::TXScopeTaggedObjPointerBase<_Ty, TagID>(scpobj_ref) {}
			TXScopeTaggedObjPointer(const TXScopeTaggedObjPointer& src_cref) : mse::rsv::us::impl::TXScopeTaggedObjPointerBase<_Ty, TagID>(
				static_cast<const mse::rsv::us::impl::TXScopeTaggedObjPointerBase<_Ty, TagID>&>(src_cref)) {}
			//template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			//TXScopeTaggedObjPointer(const TXScopeTaggedObjPointer<_Ty2>& src_cref) : mse::rsv::us::impl::TXScopeTaggedObjPointerBase<_Ty, TagID>(mse::rsv::us::impl::TXScopeTaggedObjPointerBase<_Ty2>(src_cref)) {}
			TXScopeTaggedObjPointer<_Ty, TagID>& operator=(TXScopeTaggedObj<_Ty, TagID>* ptr) {
				return mse::rsv::us::impl::TXScopeTaggedObjPointerBase<_Ty, TagID>::operator=(ptr);
			}
			TXScopeTaggedObjPointer<_Ty, TagID>& operator=(const TXScopeTaggedObjPointer<_Ty, TagID>& _Right_cref) {
				return mse::rsv::us::impl::TXScopeTaggedObjPointerBase<_Ty, TagID>::operator=(_Right_cref);
			}
			operator bool() const {
				bool retval = (bool(*static_cast<const mse::rsv::us::impl::TXScopeTaggedObjPointerBase<_Ty, TagID>*>(this)));
				return retval;
			}
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			MSE_DEPRECATED explicit operator _Ty* () const {
				_Ty* retval = std::addressof(*(*this))/*(*static_cast<const mse::rsv::us::impl::TXScopeTaggedObjPointerBase<_Ty, TagID>*>(this))*/;
				return retval;
			}
			MSE_DEPRECATED explicit operator TXScopeTaggedObj<_Ty, TagID>* () const {
				TXScopeTaggedObj<_Ty, TagID>* retval = (*static_cast<const mse::rsv::us::impl::TXScopeTaggedObjPointerBase<_Ty, TagID>*>(this));
				return retval;
			}

			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			friend class TXScopeTaggedObjNotNullPointer<_Ty, TagID>;
			friend class us::impl::TCommonizedPointer<_Ty, TXScopeTaggedObjPointer<_Ty, TagID> >;
			friend class us::impl::TCommonizedConstPointer<const _Ty, TXScopeTaggedObjPointer<_Ty, TagID> >;
		};

		/* Use TXScopeTaggedObjFixedConstPointer instead. */
		template<typename _Ty, size_t TagID>
		class TXScopeTaggedObjConstPointer : public mse::rsv::us::impl::TXScopeTaggedObjConstPointerBase<_Ty, TagID>, public mse::us::impl::XScopeTaggedContainsNonOwningScopeTaggedReferenceTagBase, public mse::us::impl::StrongPointerAsyncNotShareableAndNotPassableTagBase {
		public:
			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedObjConstPointer() {}
		private:
			TXScopeTaggedObjConstPointer() : mse::rsv::us::impl::TXScopeTaggedObjConstPointerBase<_Ty, TagID>() {}
			TXScopeTaggedObjConstPointer(const TXScopeTaggedObj<_Ty, TagID>& scpobj_cref) : mse::rsv::us::impl::TXScopeTaggedObjConstPointerBase<_Ty, TagID>(scpobj_cref) {}
			TXScopeTaggedObjConstPointer(const TXScopeTaggedObjConstPointer& src_cref) : mse::rsv::us::impl::TXScopeTaggedObjConstPointerBase<_Ty, TagID>(
				static_cast<const mse::rsv::us::impl::TXScopeTaggedObjConstPointerBase<_Ty, TagID>&>(src_cref)) {}
			//template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			//TXScopeTaggedObjConstPointer(const TXScopeTaggedObjConstPointer<_Ty2>& src_cref) : mse::rsv::us::impl::TXScopeTaggedObjConstPointerBase<_Ty, TagID>(src_cref) {}
			TXScopeTaggedObjConstPointer(const TXScopeTaggedObjPointer<_Ty, TagID>& src_cref) : mse::rsv::us::impl::TXScopeTaggedObjConstPointerBase<_Ty, TagID>(src_cref) {}
			//template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			//TXScopeTaggedObjConstPointer(const TXScopeTaggedObjPointer<_Ty2>& src_cref) : mse::rsv::us::impl::TXScopeTaggedObjConstPointerBase<_Ty, TagID>(mse::rsv::us::impl::TXScopeTaggedItemConstPointerBase<_Ty2, TagID2>(src_cref)) {}
			TXScopeTaggedObjConstPointer<_Ty, TagID>& operator=(const TXScopeTaggedObj<_Ty, TagID>* ptr) {
				return mse::rsv::us::impl::TXScopeTaggedItemConstPointerBase<_Ty, TagID>::operator=(ptr);
			}
			TXScopeTaggedObjConstPointer<_Ty, TagID>& operator=(const TXScopeTaggedObjConstPointer<_Ty, TagID>& _Right_cref) {
				return mse::rsv::us::impl::TXScopeTaggedItemConstPointerBase<_Ty, TagID>::operator=(_Right_cref);
			}
			operator bool() const {
				bool retval = (bool(*static_cast<const mse::rsv::us::impl::TXScopeTaggedObjConstPointerBase<_Ty, TagID>*>(this)));
				return retval;
			}
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			MSE_DEPRECATED explicit operator const _Ty* () const {
				const _Ty* retval = (*static_cast<const mse::rsv::us::impl::TXScopeTaggedObjConstPointerBase<_Ty, TagID>*>(this));
				return retval;
			}
			MSE_DEPRECATED explicit operator const TXScopeTaggedObj<_Ty, TagID>* () const {
				const TXScopeTaggedObj<_Ty, TagID>* retval = (*static_cast<const mse::rsv::us::impl::TXScopeTaggedObjConstPointerBase<_Ty, TagID>*>(this));
				return retval;
			}

			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			friend class TXScopeTaggedObjNotNullConstPointer<_Ty, TagID>;
			friend class us::impl::TCommonizedConstPointer<const _Ty, TXScopeTaggedObjConstPointer<_Ty, TagID> >;
		};

		/* Use TXScopeTaggedObjFixedPointer instead. */
		template<typename _Ty, size_t TagID>
		class TXScopeTaggedObjNotNullPointer : public TXScopeTaggedObjPointer<_Ty, TagID>, public mse::us::impl::NeverNullTagBase {
		public:
			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedObjNotNullPointer() {}
		private:
			TXScopeTaggedObjNotNullPointer(TXScopeTaggedObj<_Ty, TagID>& scpobj_ref) : TXScopeTaggedObjPointer<_Ty, TagID>(scpobj_ref) {}
			TXScopeTaggedObjNotNullPointer(TXScopeTaggedObj<_Ty, TagID>* ptr) : TXScopeTaggedObjPointer<_Ty, TagID>(ptr) {}
			//template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			//TXScopeTaggedObjNotNullPointer(const TXScopeTaggedObjNotNullPointer<_Ty2>& src_cref) : TXScopeTaggedObjPointer<_Ty, TagID>(src_cref) {}
			TXScopeTaggedObjNotNullPointer<_Ty, TagID>& operator=(const TXScopeTaggedObjPointer<_Ty, TagID>& _Right_cref) {
				TXScopeTaggedObjPointer<_Ty, TagID>::operator=(_Right_cref);
				return (*this);
			}
			operator bool() const { return (*static_cast<const TXScopeTaggedObjPointer<_Ty, TagID>*>(this)); }
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			MSE_DEPRECATED explicit operator _Ty* () const { return TXScopeTaggedObjPointer<_Ty, TagID>::operator _Ty * (); }
			MSE_DEPRECATED explicit operator TXScopeTaggedObj<_Ty, TagID>* () const { return TXScopeTaggedObjPointer<_Ty, TagID>::operator TXScopeTaggedObj<_Ty, TagID> * (); }

			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			friend class TXScopeTaggedObjFixedPointer<_Ty, TagID>;
		};

		/* Use TXScopeTaggedObjFixedConstPointer instead. */
		template<typename _Ty, size_t TagID>
		class TXScopeTaggedObjNotNullConstPointer : public TXScopeTaggedObjConstPointer<_Ty, TagID>, public mse::us::impl::NeverNullTagBase {
		public:
			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedObjNotNullConstPointer() {}
		private:
			TXScopeTaggedObjNotNullConstPointer(const TXScopeTaggedObjNotNullConstPointer<_Ty, TagID>& src_cref) : TXScopeTaggedObjConstPointer<_Ty, TagID>(src_cref) {}
			//template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			//TXScopeTaggedObjNotNullConstPointer(const TXScopeTaggedObjNotNullConstPointer<_Ty2>& src_cref) : TXScopeTaggedObjConstPointer<_Ty, TagID>(src_cref) {}
			TXScopeTaggedObjNotNullConstPointer(const TXScopeTaggedObjNotNullPointer<_Ty, TagID>& src_cref) : TXScopeTaggedObjConstPointer<_Ty, TagID>(src_cref) {}
			//template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			//TXScopeTaggedObjNotNullConstPointer(const TXScopeTaggedObjNotNullPointer<_Ty2>& src_cref) : TXScopeTaggedObjConstPointer<_Ty, TagID>(src_cref) {}
			TXScopeTaggedObjNotNullConstPointer(const TXScopeTaggedObj<_Ty, TagID>& scpobj_cref) : TXScopeTaggedObjConstPointer<_Ty, TagID>(scpobj_cref) {}
			operator bool() const { return (*static_cast<const TXScopeTaggedObjConstPointer<_Ty, TagID>*>(this)); }
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			MSE_DEPRECATED explicit operator const _Ty* () const { return TXScopeTaggedObjConstPointer<_Ty, TagID>::operator const _Ty * (); }
			MSE_DEPRECATED explicit operator const TXScopeTaggedObj<_Ty, TagID>* () const { return TXScopeTaggedObjConstPointer<_Ty, TagID>::operator const TXScopeTaggedObj<_Ty, TagID> * (); }

			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			friend class TXScopeTaggedObjFixedConstPointer<_Ty, TagID>;
		};

		namespace impl {
			template<typename _Ty>
			auto pointer_to_non_const_lvalue(const _Ty& x) { return &const_cast<_Ty&>(x); };
			template<typename _Ty>
			auto pointer_to_const_lvalue(const _Ty& x) { return &x; };

			template<typename _Ty> using adjusted1_TXScopeObjFixedPointer = decltype(pointer_to_non_const_lvalue(std::declval<mse::TXScopeObj<_Ty> >()));
			template<typename _Ty> using adjusted1_TXScopeObjFixedConstPointer = decltype(pointer_to_const_lvalue(std::declval<typename std::add_const<mse::TXScopeObj<_Ty> >::type>()));
		}

		/* A TXScopeTaggedObjFixedPointer points to a TXScopeTaggedObj. Its intended for very limited use. Basically just to pass a TXScopeTaggedObj
		by reference as a function parameter. TXScopeTaggedObjFixedPointers can be obtained from TXScopeTaggedObj's "&" (address of) operator. */
		template<typename _Ty, size_t TagID>
		class TXScopeTaggedObjFixedPointer : public impl::adjusted1_TXScopeObjFixedPointer<_Ty> {
		public:
			typedef impl::adjusted1_TXScopeObjFixedPointer<_Ty> base_class;
			TXScopeTaggedObjFixedPointer(const TXScopeTaggedObjFixedPointer& src_cref) = default;
			//template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			//TXScopeTaggedObjFixedPointer(const TXScopeTaggedObjFixedPointer<_Ty2, TagID2>& src_cref) : TXScopeTaggedObjNotNullPointer<_Ty, TagID>(src_cref) {}
			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedObjFixedPointer() {}
			operator bool() const { return (*static_cast<const base_class*>(this)); }
			void xscope_tagged_tag() const {}

		private:
			TXScopeTaggedObjFixedPointer(TXScopeTaggedObj<_Ty, TagID>& scpobj_ref) : base_class(&(static_cast<mse::TXScopeObj<_Ty>&>(scpobj_ref))) {}
#ifdef MSE_SCOPE_TAGGED_DISABLE_MOVE_RESTRICTIONS
			/* Disabling public move construction prevents some unsafe operations, like some, but not all,
			attempts to use a TXScopeTaggedObjFixedPointer<> as a return value. But it also prevents some safe
			operations too, like initialization via '=' (assignment operator). And the set of prevented
			operations might not be consistent across compilers. We're deciding here that the minor safety
			benefits outweigh the minor inconveniences. Note that we can't disable public move construction
			in the other scope pointers as it would interfere with implicit conversions. */
			TXScopeTaggedObjFixedPointer(TXScopeTaggedObjFixedPointer&& src_ref) : base_class(src_ref) {
				int q = 5;
			}
#endif // !MSE_SCOPE_TAGGED_DISABLE_MOVE_RESTRICTIONS
			TXScopeTaggedObjFixedPointer<_Ty, TagID>& operator=(const TXScopeTaggedObjFixedPointer<_Ty, TagID>& _Right_cref) = delete;
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			friend class TXScopeTaggedObj<_Ty, TagID>;
		};

		template<typename _Ty, size_t TagID>
		class TXScopeTaggedObjFixedConstPointer : public impl::adjusted1_TXScopeObjFixedConstPointer<_Ty> {
		public:
			typedef impl::adjusted1_TXScopeObjFixedConstPointer<_Ty> base_class;
			TXScopeTaggedObjFixedConstPointer(const TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& src_cref) = default;
			TXScopeTaggedObjFixedConstPointer(const TXScopeTaggedObjFixedPointer<_Ty, TagID>& src_cref) : base_class(src_cref) {}
			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedObjFixedConstPointer() {}
			operator bool() const { return (*static_cast<const base_class*>(this)); }
			void xscope_tagged_tag() const {}

		private:
			TXScopeTaggedObjFixedConstPointer(const TXScopeTaggedObj<_Ty, TagID>& scpobj_cref) : base_class(&(static_cast<const mse::TXScopeObj<_Ty>&>(scpobj_cref))) {}
			TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& operator=(const TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& _Right_cref) = delete;
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			friend class TXScopeTaggedObj<_Ty, TagID>;
		};

		/* TXScopeTaggedObj is intended as a transparent wrapper for other classes/objects with "scope lifespans". That is, objects
		that are either allocated on the stack, or whose "owning" pointer is allocated on the stack. */
		template<typename _TROy, size_t TagID>
		class TXScopeTaggedObj : public mse::TXScopeObj<_TROy>
			//, public MSE_FIRST_OR_PLACEHOLDER_IF_A_BASE_OF_SECOND(mse::us::impl::XScopeTaggedTagBase, mse::TXScopeObj<_TROy>, TXScopeTaggedObj<_TROy, TagID>)
			//, public MSE_FIRST_OR_PLACEHOLDER_IF_A_BASE_OF_SECOND(mse::us::impl::ReferenceableByScopeTaggedPointerTagBase, mse::TXScopeObj<_TROy>, TXScopeTaggedObj<_TROy, TagID>)
		{
		public:
			typedef mse::TXScopeObj<_TROy> base_class;

			TXScopeTaggedObj(const TXScopeTaggedObj & _X) : base_class(_X) {}

#ifdef MSE_SCOPE_TAGGED_DISABLE_MOVE_RESTRICTIONS
			explicit TXScopeTaggedObj(TXScopeTaggedObj && _X) : base_class(std::forward<decltype(_X)>(_X)) {}
#endif // !MSE_SCOPE_TAGGED_DISABLE_MOVE_RESTRICTIONS

			MSE_SCOPE_TAGGED_USING(TXScopeTaggedObj, base_class);
			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedObj() {}

			TXScopeTaggedObj& operator=(TXScopeTaggedObj && _X) {
				//mse::impl::valid_if_not_rvalue_reference_of_given_type<TXScopeTaggedObj, decltype(_X)>(_X);
				base_class::operator=(std::forward<decltype(_X)>(_X));
				return (*this);
			}
			TXScopeTaggedObj& operator=(const TXScopeTaggedObj & _X) { base_class::operator=(_X); return (*this); }
			template<class _Ty2, size_t TagID2>
			TXScopeTaggedObj& operator=(_Ty2 && _X) {
				base_class::operator=(std::forward<decltype(_X)>(_X));
				return (*this);
			}
			template<class _Ty2, size_t TagID2>
			TXScopeTaggedObj& operator=(const _Ty2 & _X) { base_class::operator=(_X); return (*this); }

			const TXScopeTaggedFixedPointer<_TROy, TagID> operator&()& {
				return TXScopeTaggedObjFixedPointer<_TROy, TagID>(*this);
			}
			const TXScopeTaggedFixedConstPointer<_TROy, TagID> operator&() const& {
				return TXScopeTaggedObjFixedConstPointer<_TROy, TagID>(*this);
			}
			const TXScopeTaggedFixedPointer<_TROy, TagID> mse_xscope_tagged_ifptr()& { return &(*this); }
			const TXScopeTaggedFixedConstPointer<_TROy, TagID> mse_xscope_tagged_ifptr() const& { return &(*this); }

			TXScopeTaggedCagedItemFixedConstPointerToRValue<_TROy, TagID> operator&()&& {
				//return TXScopeTaggedFixedConstPointer<_TROy, TagID>(TXScopeTaggedObjFixedPointer<_TROy, TagID>(&(*static_cast<base_class*>(this))));
				return TXScopeTaggedFixedConstPointer<_TROy, TagID>(TXScopeTaggedObjFixedPointer<_TROy, TagID>(*this));
			}
			TXScopeTaggedCagedItemFixedConstPointerToRValue<_TROy, TagID> operator&() const&& {
				return TXScopeTaggedObjFixedConstPointer<_TROy, TagID>(TXScopeTaggedObjFixedConstPointer<_TROy, TagID>(&(*static_cast<const base_class*>(this))));
			}
			const TXScopeTaggedCagedItemFixedConstPointerToRValue<_TROy, TagID> mse_xscope_tagged_ifptr()&& { return &(*this); }
			const TXScopeTaggedCagedItemFixedConstPointerToRValue<_TROy, TagID> mse_xscope_tagged_ifptr() const&& { return &(*this); }

			void xscope_tagged_tag() const {}
			//void xscope_tagged_contains_accessible_scope_tagged_address_of_operator_tag() const {}
			/* This type can be safely used as a function return value if _Ty is also safely returnable. */
			template<class _Ty2 = _TROy, size_t TagID2, class = typename std::enable_if<(std::is_same<_Ty2, _TROy>::value) && (
				(std::integral_constant<bool, mse::impl::HasXScopeTaggedReturnableTagMethod<_Ty2>::Has>()) || (mse::impl::is_potentially_not_xscope<_Ty2>::value)
				), void>::type>
				void xscope_tagged_returnable_tag() const {} /* Indication that this type is can be used as a function return value. */

		private:
			MSE_DEFAULT_OPERATOR_NEW_DECLARATION

				template<typename _TROy2, size_t ID2>
			friend class TXScopeTaggedOwnerPointer;
			//friend class TXScopeTaggedOwnerPointer<_TROy>;
		};

		template<typename _Ty, size_t TagID>
		auto xscope_tagged_ifptr_to(_Ty&& _X) {
			return _X.mse_xscope_tagged_ifptr();
		}
		template<typename _Ty, size_t TagID>
		auto xscope_tagged_ifptr_to(const _Ty& _X) {
			return _X.mse_xscope_tagged_ifptr();
		}


		namespace us {
			/* A couple of unsafe functions for internal use. */
			template<size_t TagID, typename _Ty>
			TXScopeTaggedFixedPointer<_Ty, TagID> unsafe_make_xscope_tagged_pointer_to(_Ty& ref);
			template<size_t TagID, typename _Ty>
			TXScopeTaggedFixedConstPointer<_Ty, TagID> unsafe_make_xscope_tagged_const_pointer_to(const _Ty& cref);
		}


		namespace impl {
			/* This template type alias is only used because msvc2017(v15.9.0) crashes if the type expression is used directly. */
			template<class _Ty2, size_t TagID, class _TMemberObjectPointer>
			using make_xscope_tagged_pointer_to_member_v2_return_type1 = TXScopeTaggedFixedPointer<typename std::remove_reference<decltype(std::declval<_Ty2>().*std::declval<_TMemberObjectPointer>())>::type, TagID>;
		}

		/* While TXScopeTaggedObjFixedPointer<> points to a TXScopeTaggedObj<>, TXScopeTaggedFixedPointer<> is intended to be able to point to a
		TXScopeTaggedObj<>, any member of a TXScopeTaggedObj<>, or various other items with scope lifetime that, for various reasons, aren't
		declared as TXScopeTaggedObj<>. */
		template<typename _Ty, size_t TagID>
		class TXScopeTaggedFixedPointer : public mse::TXScopeFixedPointer<_Ty> {
		public:
			typedef mse::TXScopeFixedPointer<_Ty> base_class;
			static const size_t s_TagID = TagID;
			MSE_SCOPE_TAGGED_USING(TXScopeTaggedFixedPointer, base_class);
			TXScopeTaggedFixedPointer(const TXScopeTaggedFixedPointer& src_cref) = default;
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedPointer(const TXScopeTaggedFixedPointer<_Ty2, TagID2>& src_cref) : base_class(src_cref) {}

			TXScopeTaggedFixedPointer(const TXScopeTaggedObjFixedPointer<_Ty, TagID>& src_cref) : base_class(static_cast<const base_class&>(src_cref)) {}
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedPointer(const TXScopeTaggedObjFixedPointer<_Ty2, TagID2>& src_cref) : base_class(src_cref) {}

			//TXScopeTaggedFixedPointer(const TXScopeTaggedOwnerPointer<_Ty>& src_cref) : TXScopeTaggedFixedPointer(&(*src_cref)) {}
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedPointer(const TXScopeTaggedOwnerPointer<_Ty2, TagID2>& src_cref) : TXScopeTaggedFixedPointer(&(*src_cref)) {}

			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedFixedPointer() {}

			operator bool() const { return true; }
			/* This native pointer cast operator is just for compatibility with existing/legacy code and ideally should never be used. */
			MSE_DEPRECATED explicit operator _Ty* () const { return std::addressof(*(*this))/*base_class::operator _Ty*()*/; }
			void xscope_tagged_tag() const {}

		private:
			TXScopeTaggedFixedPointer(_Ty* ptr) : base_class(mse::us::unsafe_make_xscope_pointer_to(*ptr)) {}
			TXScopeTaggedFixedPointer(const base_class& ptr) : base_class(ptr) {}
			TXScopeTaggedFixedPointer<_Ty, TagID>& operator=(const TXScopeTaggedFixedPointer<_Ty, TagID>& _Right_cref) = delete;
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			template<class _Ty2, size_t TagID2, class _TMemberObjectPointer>
			friend auto make_xscope_tagged_pointer_to_member_v2(const TXScopeTaggedFixedPointer<_Ty2, TagID2>& lease_pointer, const _TMemberObjectPointer& member_object_ptr)
				->mse::rsv::impl::make_xscope_tagged_pointer_to_member_v2_return_type1<_Ty2, TagID2, _TMemberObjectPointer>;
			/* These versions of make_xscope_tagged_pointer_to_member() are actually now deprecated. */
			template<class _TTargetType, class _Ty2, size_t TagID2>
			friend TXScopeTaggedFixedPointer<_TTargetType, TagID2> make_xscope_tagged_pointer_to_member(_TTargetType& target, const TXScopeTaggedObjFixedPointer<_Ty2, TagID2>& lease_pointer);
			template<class _TTargetType, class _Ty2, size_t TagID2>
			friend TXScopeTaggedFixedPointer<_TTargetType, TagID2> make_xscope_tagged_pointer_to_member(_TTargetType& target, const TXScopeTaggedFixedPointer<_Ty2, TagID2>& lease_pointer);

			template<size_t TagID2, class _Ty2> friend TXScopeTaggedFixedPointer<_Ty2, TagID2> us::unsafe_make_xscope_tagged_pointer_to(_Ty2& ref);
		};

		template<typename _Ty, size_t TagID>
		class TXScopeTaggedFixedConstPointer : public mse::TXScopeFixedConstPointer<_Ty> {
		public:
			typedef mse::TXScopeFixedConstPointer<_Ty> base_class;
			static const size_t s_TagID = TagID;
			MSE_SCOPE_TAGGED_USING(TXScopeTaggedFixedConstPointer, base_class);
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedConstPointer<_Ty, TagID>& src_cref) = default;
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedConstPointer<_Ty2, TagID2>& src_cref) : base_class(src_cref) {}

			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedPointer<_Ty, TagID>& src_cref) : base_class(src_cref) {}
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedPointer<_Ty2, TagID2>& src_cref) : base_class(src_cref) {}

			TXScopeTaggedFixedConstPointer(const TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& src_cref) : base_class(src_cref) {}
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedObjFixedConstPointer<_Ty2, TagID2>& src_cref) : base_class(src_cref) {}

			TXScopeTaggedFixedConstPointer(const TXScopeTaggedObjFixedPointer<_Ty, TagID>& src_cref) : base_class(src_cref) {}
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedObjFixedPointer<_Ty2, TagID2>& src_cref) : base_class(src_cref) {}

			//TXScopeTaggedFixedConstPointer(const TXScopeTaggedOwnerPointer<_Ty>& src_cref) : TXScopeTaggedFixedConstPointer(&(*src_cref)) {}
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedOwnerPointer<_Ty2, TagID2>& src_cref) : TXScopeTaggedFixedConstPointer(&(*src_cref)) {}

			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedFixedConstPointer() {}

			operator bool() const { return true; }
			void xscope_tagged_tag() const {}

		private:
			TXScopeTaggedFixedConstPointer(const _Ty* ptr) : base_class(mse::us::unsafe_make_xscope_const_pointer_to(*ptr)) {}
			TXScopeTaggedFixedConstPointer(const base_class& ptr) : base_class(ptr) {}
			TXScopeTaggedFixedConstPointer<_Ty, TagID>& operator=(const TXScopeTaggedFixedConstPointer<_Ty, TagID>& _Right_cref) = delete;
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			template<class _Ty2, size_t TagID2, class _TMemberObjectPointer>
			friend auto make_xscope_tagged_pointer_to_member_v2(const TXScopeTaggedFixedConstPointer<_Ty2, TagID2>& lease_pointer, const _TMemberObjectPointer& member_object_ptr)
				->TXScopeTaggedFixedConstPointer<typename std::remove_const<typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type>::type, TagID2>;
			template<class _Ty2, size_t TagID2, class _TMemberObjectPointer>
			friend auto make_xscope_tagged_const_pointer_to_member_v2(const TXScopeTaggedFixedPointer<_Ty2, TagID2>& lease_pointer, const _TMemberObjectPointer& member_object_ptr)
				->TXScopeTaggedFixedConstPointer<typename std::remove_const<typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type>::type, TagID2>;
			template<class _Ty2, size_t TagID2, class _TMemberObjectPointer>
			friend auto make_xscope_tagged_const_pointer_to_member_v2(const TXScopeTaggedFixedConstPointer<_Ty2, TagID2>& lease_pointer, const _TMemberObjectPointer& member_object_ptr)
				->TXScopeTaggedFixedConstPointer<typename std::remove_const<typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type>::type, TagID2>;
			/* These versions of make_xscope_tagged_pointer_to_member() are actually now deprecated. */
			template<class _TTargetType, class _Ty2, size_t TagID2>
			friend TXScopeTaggedFixedConstPointer<_TTargetType, TagID2> make_xscope_tagged_pointer_to_member(const _TTargetType& target, const TXScopeTaggedObjFixedConstPointer<_Ty2, TagID2>& lease_pointer);
			template<class _TTargetType, class _Ty2, size_t TagID2>
			friend TXScopeTaggedFixedConstPointer<_TTargetType, TagID2> make_xscope_tagged_const_pointer_to_member(const _TTargetType& target, const TXScopeTaggedObjFixedPointer<_Ty2, TagID2>& lease_pointer);
			template<class _TTargetType, class _Ty2, size_t TagID2>
			friend TXScopeTaggedFixedConstPointer<_TTargetType, TagID2> make_xscope_tagged_const_pointer_to_member(const _TTargetType& target, const TXScopeTaggedObjFixedConstPointer<_Ty2, TagID2>& lease_pointer);

			template<class _TTargetType, class _Ty2, size_t TagID2>
			friend TXScopeTaggedFixedConstPointer<_TTargetType, TagID2> make_xscope_tagged_pointer_to_member(const _TTargetType& target, const TXScopeTaggedFixedConstPointer<_Ty2, TagID2>& lease_pointer);
			template<class _TTargetType, class _Ty2, size_t TagID2>
			friend TXScopeTaggedFixedConstPointer<_TTargetType, TagID2> make_xscope_tagged_const_pointer_to_member(const _TTargetType& target, const TXScopeTaggedFixedPointer<_Ty2, TagID2>& lease_pointer);
			template<class _TTargetType, class _Ty2, size_t TagID2>
			friend TXScopeTaggedFixedConstPointer<_TTargetType, TagID2> make_xscope_tagged_const_pointer_to_member(const _TTargetType& target, const TXScopeTaggedFixedConstPointer<_Ty2, TagID2>& lease_pointer);
			template<size_t TagID2, class _Ty2> friend TXScopeTaggedFixedConstPointer<_Ty2, TagID2> us::unsafe_make_xscope_tagged_const_pointer_to(const _Ty2& cref);
		};

		/* TXScopeTaggedCagedItemFixedPointerToRValue<> holds a TXScopeTaggedFixedPointer<> that points to an r-value which can only be
		accessed when converted to a rsv::TXScopeTaggedFixedPointerFParam<>. */
		template<typename _Ty, size_t TagID>
		class TXScopeTaggedCagedItemFixedPointerToRValue : public mse::us::impl::XScopeTaggedContainsNonOwningScopeTaggedReferenceTagBase, public mse::us::impl::StrongPointerAsyncNotShareableAndNotPassableTagBase {
		public:
			void xscope_tagged_tag() const {}

		private:
			TXScopeTaggedCagedItemFixedPointerToRValue(const TXScopeTaggedCagedItemFixedPointerToRValue&) = delete;
			TXScopeTaggedCagedItemFixedPointerToRValue(TXScopeTaggedCagedItemFixedPointerToRValue&&) = default;
			TXScopeTaggedCagedItemFixedPointerToRValue(const TXScopeTaggedFixedPointer<_Ty, TagID>& ptr) : m_xscope_tagged_ptr(ptr) {}

			auto uncaged_pointer() const {
				return m_xscope_tagged_ptr;
			}

			TXScopeTaggedCagedItemFixedPointerToRValue<_Ty, TagID>& operator=(const TXScopeTaggedCagedItemFixedPointerToRValue<_Ty, TagID>& _Right_cref) = delete;
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			TXScopeTaggedFixedPointer<_Ty, TagID> m_xscope_tagged_ptr;

			friend class TXScopeTaggedObj<_Ty, TagID>;
			template<class _Ty2, size_t TagID2> friend class rsv::TXScopeTaggedFixedPointerFParam;
			template<class _Ty2, size_t TagID2> friend class rsv::TXScopeTaggedFixedConstPointerFParam;
			template<class _Ty2, size_t TagID2> friend class TXScopeTaggedStrongPointerStore;
		};

		template<typename _Ty, size_t TagID>
		class TXScopeTaggedCagedItemFixedConstPointerToRValue : public mse::us::impl::XScopeTaggedContainsNonOwningScopeTaggedReferenceTagBase, public mse::us::impl::StrongPointerAsyncNotShareableAndNotPassableTagBase {
		public:
			void xscope_tagged_tag() const {}

		private:
			TXScopeTaggedCagedItemFixedConstPointerToRValue(const TXScopeTaggedCagedItemFixedConstPointerToRValue& src_cref) = delete;
			TXScopeTaggedCagedItemFixedConstPointerToRValue(TXScopeTaggedCagedItemFixedConstPointerToRValue&& src_cref) = default;
			TXScopeTaggedCagedItemFixedConstPointerToRValue(const TXScopeTaggedFixedConstPointer<_Ty, TagID>& ptr) : m_xscope_tagged_ptr(ptr) {}
#ifdef MSE_SCOPE_TAGGED_DISABLE_MOVE_RESTRICTIONS
			TXScopeTaggedCagedItemFixedConstPointerToRValue(TXScopeTaggedCagedItemFixedConstPointerToRValue&& src_ref) : m_xscope_tagged_ptr(src_ref) {}
#endif // !MSE_SCOPE_TAGGED_DISABLE_MOVE_RESTRICTIONS

			auto uncaged_pointer() const { return m_xscope_tagged_ptr; }

			TXScopeTaggedCagedItemFixedConstPointerToRValue<_Ty, TagID>& operator=(const TXScopeTaggedCagedItemFixedConstPointerToRValue<_Ty, TagID>& _Right_cref) = delete;
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

			TXScopeTaggedFixedConstPointer<_Ty, TagID> m_xscope_tagged_ptr;

			friend class TXScopeTaggedObj<_Ty, TagID>;
			template<class _Ty2, size_t TagID2> friend class rsv::TXScopeTaggedFixedConstPointerFParam;
			template<typename _Ty2> friend auto pointer_to(_Ty2&& _X) -> decltype(&std::forward<_Ty2>(_X));
			template<class _Ty2, size_t TagID2> friend class TXScopeTaggedStrongConstPointerStore;
		};
	}
}

namespace std {
	template<class _Ty, size_t TagID>
	struct hash<mse::rsv::TXScopeTaggedFixedPointer<_Ty, TagID> > {	// hash functor
		typedef mse::rsv::TXScopeTaggedFixedPointer<_Ty, TagID> argument_type;
		typedef size_t result_type;
		size_t operator()(const mse::rsv::TXScopeTaggedFixedPointer<_Ty, TagID>& _Keyval) const _NOEXCEPT {
			const _Ty* ptr1 = nullptr;
			if (_Keyval) {
				ptr1 = std::addressof(*_Keyval);
			}
			return (hash<const _Ty *>()(ptr1));
		}
	};
	template<class _Ty, size_t TagID>
	struct hash<mse::rsv::TXScopeTaggedObjFixedPointer<_Ty, TagID> > {	// hash functor
		typedef mse::rsv::TXScopeTaggedObjFixedPointer<_Ty, TagID> argument_type;
		typedef size_t result_type;
		size_t operator()(const mse::rsv::TXScopeTaggedObjFixedPointer<_Ty, TagID>& _Keyval) const _NOEXCEPT {
			const _Ty* ptr1 = nullptr;
			if (_Keyval) {
				ptr1 = std::addressof(*_Keyval);
			}
			return (hash<const _Ty *>()(ptr1));
		}
	};

	template<class _Ty, size_t TagID>
	struct hash<mse::rsv::TXScopeTaggedFixedConstPointer<_Ty, TagID> > {	// hash functor
		typedef mse::rsv::TXScopeTaggedFixedConstPointer<_Ty, TagID> argument_type;
		typedef size_t result_type;
		size_t operator()(const mse::rsv::TXScopeTaggedFixedConstPointer<_Ty, TagID>& _Keyval) const _NOEXCEPT {
			const _Ty* ptr1 = nullptr;
			if (_Keyval) {
				ptr1 = std::addressof(*_Keyval);
			}
			return (hash<const _Ty *>()(ptr1));
		}
	};
	template<class _Ty, size_t TagID>
	struct hash<mse::rsv::TXScopeTaggedObjFixedConstPointer<_Ty, TagID> > {	// hash functor
		typedef mse::rsv::TXScopeTaggedObjFixedConstPointer<_Ty, TagID> argument_type;
		typedef size_t result_type;
		size_t operator()(const mse::rsv::TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& _Keyval) const _NOEXCEPT {
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
		template<typename _TROy>
		class TNonXScopeTaggedObj : public _TROy {
		public:
			MSE_USING(TNonXScopeTaggedObj, _TROy);
			TNonXScopeTaggedObj(const TNonXScopeTaggedObj& _X) : _TROy(_X) {}
			TNonXScopeTaggedObj(TNonXScopeTaggedObj&& _X) : _TROy(std::forward<decltype(_X)>(_X)) {}
			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TNonXScopeTaggedObj() {
				//mse::impl::T_valid_if_not_an_xscope_tagged_type<_TROy>();
				mse::impl::T_valid_if_not_an_xscope_type<_TROy>();
			}

			TNonXScopeTaggedObj& operator=(TNonXScopeTaggedObj&& _X) { _TROy::operator=(std::forward<decltype(_X)>(_X)); return (*this); }
			TNonXScopeTaggedObj& operator=(const TNonXScopeTaggedObj& _X) { _TROy::operator=(_X); return (*this); }
			template<class _Ty2, size_t TagID2>
			TNonXScopeTaggedObj& operator=(_Ty2&& _X) { _TROy::operator=(std::forward<decltype(_X)>(_X)); return (*this); }
			//TNonXScopeTaggedObj& operator=(_Ty2&& _X) { static_cast<_TROy&>(*this) = (std::forward<decltype(_X)>(_X)); return (*this); }
			template<class _Ty2, size_t TagID2>
			TNonXScopeTaggedObj& operator=(const _Ty2& _X) { _TROy::operator=(_X); return (*this); }

		private:
			MSE_DEFAULT_OPERATOR_AMPERSAND_DECLARATION;
		};


		/* template specializations */

#if 0

#define MSE_SCOPE_TAGGED_IMPL_OBJ_INHERIT_ASSIGNMENT_OPERATOR(class_name) \
		auto& operator=(class_name&& _X) { base_class::operator=(std::forward<decltype(_X)>(_X)); return (*this); } \
		auto& operator=(const class_name& _X) { base_class::operator=(_X); return (*this); } \
		template<class _Ty2, size_t TagID2> auto& operator=(_Ty2&& _X) { base_class::operator=(std::forward<decltype(_X)>(_X)); return (*this); } \
		template<class _Ty2, size_t TagID2> auto& operator=(const _Ty2& _X) { base_class::operator=(_X); return (*this); }

#define MSE_SCOPE_TAGGED_IMPL_OBJ_SPECIALIZATION_DEFINITIONS1(class_name) \
		class_name(const class_name&) = default; \
		class_name(class_name&&) = default; \
		MSE_SCOPE_TAGGED_IMPL_OBJ_INHERIT_ASSIGNMENT_OPERATOR(class_name);

#if !defined(MSE_SOME_POINTER_TYPE_IS_DISABLED)
#define MSE_SCOPE_TAGGED_IMPL_OBJ_NATIVE_POINTER_PRIVATE_CONSTRUCTORS1(class_name) \
			class_name(std::nullptr_t) {} \
			class_name() {}
#else // !defined(MSE_SOME_POINTER_TYPE_IS_DISABLED)
#define MSE_SCOPE_TAGGED_IMPL_OBJ_NATIVE_POINTER_PRIVATE_CONSTRUCTORS1(class_name)
#endif // !defined(MSE_SOME_POINTER_TYPE_IS_DISABLED)

	/* Note that because we explicitly define some (private) constructors, default copy and move constructors
	and assignment operators won't be generated, so we have to define those as well. */
#define MSE_SCOPE_TAGGED_IMPL_OBJ_NATIVE_POINTER_SPECIALIZATION(specified_type, mapped_type) \
		template<typename _Ty, size_t TagID> \
		class TXScopeTaggedObj<specified_type, TagID> : public TXScopeTaggedObj<mapped_type, TagID> { \
		public: \
			typedef TXScopeTaggedObj<mapped_type, TagID> base_class; \
			MSE_USING(TXScopeTaggedObj, base_class); \
			MSE_SCOPE_TAGGED_IMPL_OBJ_SPECIALIZATION_DEFINITIONS1(TXScopeTaggedObj); \
		private: \
			MSE_SCOPE_TAGGED_IMPL_OBJ_NATIVE_POINTER_PRIVATE_CONSTRUCTORS1(TXScopeTaggedObj); \
		};

	/* To achieve compatibility with the us::unsafe_make_xscope_tagged_pointer() functions, these specializations make use of
	reinterpret_cast<>s in certain situations. The safety of these reinterpret_cast<>s rely on the 'mapped_type'
	being safely "reinterpretable" as a 'specified_type'. */
#define MSE_SCOPE_TAGGED_IMPL_PTR_SPECIALIZATION(specified_type, mapped_type) \
		template<typename _Ty, size_t TagID> \
		class TXScopeTaggedFixedPointer<specified_type, TagID> : public mse::rsv::us::impl::TXScopeTaggedItemPointerBase<specified_type, TagID>, public mse::us::impl::XScopeTaggedContainsNonOwningScopeTaggedReferenceTagBase, public mse::us::impl::StrongPointerAsyncNotShareableAndNotPassableTagBase, public mse::us::impl::NeverNullTagBase { \
		public: \
			typedef mse::rsv::us::impl::TXScopeTaggedItemPointerBase<specified_type, TagID> base_class; \
			TXScopeTaggedFixedPointer(const TXScopeTaggedFixedPointer& src_cref) = default; \
			TXScopeTaggedFixedPointer(const TXScopeTaggedFixedPointer<mapped_type, TagID>& src_cref) : base_class(reinterpret_cast<const base_class&>(src_cref)) {} \
			template<size_t TagID2> \
			TXScopeTaggedFixedPointer(const TXScopeTaggedFixedPointer<mapped_type, TagID2>& src_cref) : base_class(reinterpret_cast<const base_class&>(src_cref)) {} \
			template<class specified_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<specified_type2*, specified_type*>::value, void>::type> \
			TXScopeTaggedFixedPointer(const TXScopeTaggedFixedPointer<specified_type2, TagID2> & src_cref) : base_class(static_cast<const mse::rsv::us::impl::TXScopeTaggedItemPointerBase<specified_type2, TagID2>&>(src_cref)) {} \
			TXScopeTaggedFixedPointer(const TXScopeTaggedObjFixedPointer<specified_type, TagID> & src_cref) : base_class(reinterpret_cast<const base_class&>(src_cref)) {} \
			template<class specified_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<specified_type2*, specified_type*>::value, void>::type> \
			TXScopeTaggedFixedPointer(const TXScopeTaggedObjFixedPointer<specified_type2, TagID2> & src_cref) : base_class(reinterpret_cast<const mse::rsv::us::impl::TXScopeTaggedItemPointerBase<specified_type2, TagID2>&>(src_cref)) {} \
			template<class specified_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<specified_type2*, specified_type*>::value, void>::type> \
			TXScopeTaggedFixedPointer(const TXScopeTaggedOwnerPointer<specified_type2, TagID2> & src_cref) : TXScopeTaggedFixedPointer(&(*src_cref)) {} \
			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedFixedPointer() {} \
			operator bool() const { return true; } \
			void xscope_tagged_tag() const {} \
		private: \
			TXScopeTaggedFixedPointer(specified_type * ptr) : base_class(ptr) {} \
			TXScopeTaggedFixedPointer(mapped_type * ptr) : base_class(reinterpret_cast<specified_type *>(ptr)) {} \
			TXScopeTaggedFixedPointer(const base_class & ptr) : base_class(ptr) {} \
			TXScopeTaggedFixedPointer<specified_type, TagID>& operator=(const TXScopeTaggedFixedPointer<specified_type, TagID> & _Right_cref) = delete; \
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION; \
			template<class _Ty2, size_t TagID2, class _TMemberObjectPointer> \
			friend auto make_xscope_tagged_pointer_to_member_v2(const TXScopeTaggedFixedPointer<_Ty2, TagID2> & lease_pointer, const _TMemberObjectPointer & member_object_ptr) \
				->mse::rsv::impl::make_xscope_tagged_pointer_to_member_v2_return_type1<_Ty2, TagID2, _TMemberObjectPointer>; \
			template<size_t TagID2, class _Ty2> friend TXScopeTaggedFixedPointer<_Ty2, TagID2> us::unsafe_make_xscope_tagged_pointer_to(_Ty2 & ref); \
		}; \
		template<typename _Ty, size_t TagID> \
		class TXScopeTaggedFixedConstPointer<specified_type, TagID> : public TXScopeTaggedFixedConstPointer<mapped_type, TagID> { \
		public: \
			typedef mse::rsv::us::impl::TXScopeTaggedItemConstPointerBase<specified_type, TagID> base_class; \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedConstPointer<specified_type, TagID>& src_cref) = default; \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedConstPointer<mapped_type, TagID>& src_cref) : base_class(reinterpret_cast<const base_class&>(src_cref)) {} \
			template<size_t TagID2> \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedConstPointer<mapped_type, TagID2>& src_cref) : base_class(reinterpret_cast<const base_class&>(src_cref)) {} \
			template<class specified_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<specified_type2*, specified_type*>::value, void>::type> \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedConstPointer<specified_type2, TagID2> & src_cref) : base_class(static_cast<const mse::rsv::us::impl::TXScopeTaggedItemConstPointerBase<specified_type2, TagID2>&>(src_cref)) {} \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedPointer<specified_type, TagID> & src_cref) : base_class(static_cast<const mse::rsv::us::impl::TXScopeTaggedItemPointerBase<specified_type, TagID>&>(src_cref)) {} \
			template<class specified_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<specified_type2*, specified_type*>::value, void>::type> \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedPointer<specified_type2, TagID2> & src_cref) : base_class(static_cast<const mse::rsv::us::impl::TXScopeTaggedItemPointerBase<specified_type2, TagID2>&>(src_cref)) {} \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedObjFixedConstPointer<specified_type, TagID> & src_cref) : base_class(reinterpret_cast<const base_class&>(src_cref)) {} \
			template<class specified_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<specified_type2*, specified_type*>::value, void>::type> \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedObjFixedConstPointer<specified_type2, TagID2> & src_cref) : base_class(reinterpret_cast<const mse::rsv::us::impl::TXScopeTaggedItemConstPointerBase<specified_type2, TagID2>&>(src_cref)) {} \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedObjFixedPointer<specified_type, TagID> & src_cref) : base_class(reinterpret_cast<const mse::rsv::us::impl::TXScopeTaggedItemPointerBase<specified_type, TagID>&>(src_cref)) {} \
			template<class specified_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<specified_type2*, specified_type*>::value, void>::type> \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedObjFixedPointer<specified_type2, TagID2> & src_cref) : base_class(reinterpret_cast<const mse::rsv::us::impl::TXScopeTaggedItemPointerBase<specified_type2, TagID2>&>(src_cref)) {} \
			template<class specified_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<specified_type2*, specified_type*>::value, void>::type> \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedOwnerPointer<specified_type2, TagID2> & src_cref) : TXScopeTaggedFixedConstPointer(&(*src_cref)) {} \
			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedFixedConstPointer() {} \
			operator bool() const { return true; } \
			void xscope_tagged_tag() const {} \
		private: \
			TXScopeTaggedFixedConstPointer(typename std::add_const<specified_type>::type * ptr) : base_class(ptr) {} \
			TXScopeTaggedFixedConstPointer(typename std::add_const<mapped_type>::type * ptr) : base_class(reinterpret_cast<const specified_type *>(ptr)) {} \
			TXScopeTaggedFixedConstPointer(const base_class & ptr) : base_class(ptr) {} \
			TXScopeTaggedFixedConstPointer<specified_type, TagID>& operator=(const TXScopeTaggedFixedConstPointer<specified_type, TagID> & _Right_cref) = delete; \
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION; \
			template<class specified_type2, size_t TagID2, class _TMemberObjectPointer> \
			friend auto make_xscope_tagged_pointer_to_member_v2(const TXScopeTaggedFixedConstPointer<specified_type2, TagID2> & lease_pointer, const _TMemberObjectPointer & member_object_ptr) \
				->TXScopeTaggedFixedConstPointer<typename std::remove_const<typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type>::type, TagID2>; \
			template<class specified_type2, size_t TagID2, class _TMemberObjectPointer> \
			friend auto make_xscope_tagged_const_pointer_to_member_v2(const TXScopeTaggedFixedPointer<specified_type2, TagID2> & lease_pointer, const _TMemberObjectPointer & member_object_ptr) \
				->TXScopeTaggedFixedConstPointer<typename std::remove_const<typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type>::type, TagID2>; \
			template<class specified_type2, size_t TagID2, class _TMemberObjectPointer> \
			friend auto make_xscope_tagged_const_pointer_to_member_v2(const TXScopeTaggedFixedConstPointer<specified_type2, TagID2> & lease_pointer, const _TMemberObjectPointer & member_object_ptr) \
				->TXScopeTaggedFixedConstPointer<typename std::remove_const<typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type>::type, TagID2>; \
			/* These versions of make_xscope_tagged_pointer_to_member() are actually now deprecated. */ \
			template<class _TTargetType, class specified_type2, size_t TagID2> \
			friend TXScopeTaggedFixedConstPointer<_TTargetType, TagID2> make_xscope_tagged_pointer_to_member(const _TTargetType & target, const TXScopeTaggedObjFixedConstPointer<specified_type2, TagID2> & lease_pointer); \
			template<class _TTargetType, class specified_type2, size_t TagID2> \
			friend TXScopeTaggedFixedConstPointer<_TTargetType, TagID2> make_xscope_tagged_const_pointer_to_member(const _TTargetType & target, const TXScopeTaggedObjFixedPointer<specified_type2, TagID2> & lease_pointer); \
			template<class _TTargetType, class specified_type2, size_t TagID2> \
			friend TXScopeTaggedFixedConstPointer<_TTargetType, TagID2> make_xscope_tagged_const_pointer_to_member(const _TTargetType & target, const TXScopeTaggedObjFixedConstPointer<specified_type2, TagID2> & lease_pointer); \
			template<class _TTargetType, class specified_type2, size_t TagID2> \
			friend TXScopeTaggedFixedConstPointer<_TTargetType, TagID2> make_xscope_tagged_pointer_to_member(const _TTargetType & target, const TXScopeTaggedFixedConstPointer<specified_type2, TagID2> & lease_pointer); \
			template<class _TTargetType, class specified_type2, size_t TagID2> \
			friend TXScopeTaggedFixedConstPointer<_TTargetType, TagID2> make_xscope_tagged_const_pointer_to_member(const _TTargetType & target, const TXScopeTaggedFixedPointer<specified_type2, TagID2> & lease_pointer); \
			template<class _TTargetType, class specified_type2, size_t TagID2> \
			friend TXScopeTaggedFixedConstPointer<_TTargetType, TagID2> make_xscope_tagged_const_pointer_to_member(const _TTargetType & target, const TXScopeTaggedFixedConstPointer<specified_type2, TagID2> & lease_pointer); \
			template<size_t TagID2, class _Ty2> friend TXScopeTaggedFixedConstPointer<_Ty2, TagID2> us::unsafe_make_xscope_tagged_const_pointer_to(const _Ty2 & cref); \
		};

#define MSE_SCOPE_TAGGED_IMPL_PTR_NATIVE_POINTER_SPECIALIZATION(specified_type, mapped_type) \
			MSE_SCOPE_TAGGED_IMPL_PTR_SPECIALIZATION(specified_type, mapped_type)

#define MSE_SCOPE_TAGGED_IMPL_NATIVE_POINTER_SPECIALIZATION(specified_type, mapped_type) \
		MSE_SCOPE_TAGGED_IMPL_PTR_NATIVE_POINTER_SPECIALIZATION(specified_type, mapped_type); \
		MSE_SCOPE_TAGGED_IMPL_OBJ_NATIVE_POINTER_SPECIALIZATION(specified_type, mapped_type);

		//MSE_SCOPE_TAGGED_IMPL_NATIVE_POINTER_SPECIALIZATION(_Ty*, mse::us::impl::TPointerForLegacy<_Ty>);
		//MSE_SCOPE_TAGGED_IMPL_NATIVE_POINTER_SPECIALIZATION(_Ty* const, const mse::us::impl::TPointerForLegacy<_Ty>);

#ifdef MSEPRIMITIVES_H

#define MSE_SCOPE_TAGGED_IMPL_OBJ_INTEGRAL_SPECIALIZATION(integral_type) \
		template<size_t TagID> \
		class TXScopeTaggedObj<integral_type, TagID> : public TXScopeTaggedObj<mse::TInt<integral_type>, TagID> { \
		public: \
			typedef TXScopeTaggedObj<mse::TInt<integral_type>, TagID> base_class; \
			MSE_USING(TXScopeTaggedObj, base_class); \
		};

		/* To achieve compatibility with the us::unsafe_make_xscope_tagged_pointer() functions, these specializations make use of
		reinterpret_cast<>s in certain situations. The safety of these reinterpret_cast<>s rely on 'mse::TInt<integral_type>'
		being safely "reinterpretable" as an 'integral_type'. */
#define MSE_SCOPE_TAGGED_IMPL_PTR_INTEGRAL_SPECIALIZATION(integral_type) \
		template<size_t TagID> \
		class TXScopeTaggedFixedPointer<integral_type, TagID> : public mse::rsv::us::impl::TXScopeTaggedItemPointerBase<integral_type, TagID>, public mse::us::impl::XScopeTaggedContainsNonOwningScopeTaggedReferenceTagBase, public mse::us::impl::StrongPointerAsyncNotShareableAndNotPassableTagBase, public mse::us::impl::NeverNullTagBase { \
		public: \
			typedef mse::rsv::us::impl::TXScopeTaggedItemPointerBase<integral_type, TagID> base_class; \
			TXScopeTaggedFixedPointer(const TXScopeTaggedFixedPointer& src_cref) = default; \
			TXScopeTaggedFixedPointer(const TXScopeTaggedFixedPointer<mse::TInt<integral_type>, TagID>& src_cref) : base_class(reinterpret_cast<const base_class&>(src_cref)) {} \
			template<size_t TagID2> \
			TXScopeTaggedFixedPointer(const TXScopeTaggedFixedPointer<mse::TInt<integral_type>, TagID2>& src_cref) : base_class(reinterpret_cast<const base_class&>(src_cref)) {} \
			template<class integral_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<integral_type2*, integral_type*>::value, void>::type> \
			TXScopeTaggedFixedPointer(const TXScopeTaggedFixedPointer<integral_type2, TagID2> & src_cref) : base_class(static_cast<const mse::rsv::us::impl::TXScopeTaggedItemPointerBase<integral_type2, TagID2>&>(src_cref)) {} \
			TXScopeTaggedFixedPointer(const TXScopeTaggedObjFixedPointer<integral_type, TagID> & src_cref) : base_class(reinterpret_cast<const base_class&>(src_cref)) {} \
			template<class integral_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<integral_type2*, integral_type*>::value, void>::type> \
			TXScopeTaggedFixedPointer(const TXScopeTaggedObjFixedPointer<integral_type2, TagID2> & src_cref) : base_class(reinterpret_cast<const mse::rsv::us::impl::TXScopeTaggedItemPointerBase<integral_type2, TagID2>&>(src_cref)) {} \
			template<class integral_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<integral_type2*, integral_type*>::value, void>::type> \
			TXScopeTaggedFixedPointer(const TXScopeTaggedOwnerPointer<integral_type2, TagID2> & src_cref) : TXScopeTaggedFixedPointer(&(*src_cref)) {} \
			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedFixedPointer() {} \
			operator bool() const { return true; } \
			void xscope_tagged_tag() const {} \
		private: \
			TXScopeTaggedFixedPointer(integral_type * ptr) : base_class(ptr) {} \
			TXScopeTaggedFixedPointer(mse::TInt<integral_type> * ptr) : base_class(reinterpret_cast<integral_type *>(ptr)) {} \
			TXScopeTaggedFixedPointer(const base_class & ptr) : base_class(ptr) {} \
			TXScopeTaggedFixedPointer<integral_type, TagID>& operator=(const TXScopeTaggedFixedPointer<integral_type, TagID> & _Right_cref) = delete; \
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION; \
			template<class _Ty2, size_t TagID2, class _TMemberObjectPointer> \
			friend auto make_xscope_tagged_pointer_to_member_v2(const TXScopeTaggedFixedPointer<_Ty2, TagID2> & lease_pointer, const _TMemberObjectPointer & member_object_ptr) \
				->mse::rsv::impl::make_xscope_tagged_pointer_to_member_v2_return_type1<_Ty2, TagID2, _TMemberObjectPointer>; \
			template<size_t TagID2, class _Ty2> friend TXScopeTaggedFixedPointer<_Ty2, TagID2> us::unsafe_make_xscope_tagged_pointer_to(_Ty2 & ref); \
		}; \
		template<size_t TagID> \
		class TXScopeTaggedFixedConstPointer<integral_type, TagID> : public mse::rsv::us::impl::TXScopeTaggedItemConstPointerBase<integral_type, TagID>, public mse::us::impl::XScopeTaggedContainsNonOwningScopeTaggedReferenceTagBase, public mse::us::impl::StrongPointerAsyncNotShareableAndNotPassableTagBase, public mse::us::impl::NeverNullTagBase { \
		public: \
			typedef mse::rsv::us::impl::TXScopeTaggedItemConstPointerBase<integral_type, TagID> base_class; \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedConstPointer<integral_type, TagID>& src_cref) = default; \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedConstPointer<mse::TInt<integral_type>, TagID>& src_cref) : base_class(reinterpret_cast<const base_class&>(src_cref)) {} \
			template<size_t TagID2> \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedConstPointer<mse::TInt<integral_type>, TagID2>& src_cref) : base_class(reinterpret_cast<const base_class&>(src_cref)) {} \
			template<class integral_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<integral_type2*, integral_type*>::value, void>::type> \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedConstPointer<integral_type2, TagID2> & src_cref) : base_class(static_cast<const mse::rsv::us::impl::TXScopeTaggedItemConstPointerBase<integral_type2, TagID2>&>(src_cref)) {} \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedPointer<integral_type, TagID> & src_cref) : base_class(static_cast<const mse::rsv::us::impl::TXScopeTaggedItemPointerBase<integral_type, TagID>&>(src_cref)) {} \
			template<class integral_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<integral_type2*, integral_type*>::value, void>::type> \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedFixedPointer<integral_type2, TagID2> & src_cref) : base_class(static_cast<const mse::rsv::us::impl::TXScopeTaggedItemPointerBase<integral_type2, TagID2>&>(src_cref)) {} \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedObjFixedConstPointer<integral_type, TagID> & src_cref) : base_class(reinterpret_cast<const base_class&>(src_cref)) {} \
			template<class integral_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<integral_type2*, integral_type*>::value, void>::type> \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedObjFixedConstPointer<integral_type2, TagID2> & src_cref) : base_class(reinterpret_cast<const mse::rsv::us::impl::TXScopeTaggedItemConstPointerBase<integral_type2, TagID2>&>(src_cref)) {} \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedObjFixedPointer<integral_type, TagID> & src_cref) : base_class(reinterpret_cast<const mse::rsv::us::impl::TXScopeTaggedItemPointerBase<integral_type, TagID>&>(src_cref)) {} \
			template<class integral_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<integral_type2*, integral_type*>::value, void>::type> \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedObjFixedPointer<integral_type2, TagID2> & src_cref) : base_class(static_cast<const mse::rsv::us::impl::TXScopeTaggedItemPointerBase<integral_type2, TagID2>&>(src_cref)) {} \
			template<class integral_type2, size_t TagID2, class = typename std::enable_if<std::is_convertible<integral_type2*, integral_type*>::value, void>::type> \
			TXScopeTaggedFixedConstPointer(const TXScopeTaggedOwnerPointer<integral_type2, TagID2> & src_cref) : TXScopeTaggedFixedConstPointer(&(*src_cref)) {} \
			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedFixedConstPointer() {} \
			operator bool() const { return true; } \
			void xscope_tagged_tag() const {} \
		private: \
			TXScopeTaggedFixedConstPointer(typename std::add_const<integral_type>::type * ptr) : base_class(ptr) {} \
			TXScopeTaggedFixedConstPointer(typename std::add_const<mse::TInt<integral_type>>::type * ptr) : base_class(reinterpret_cast<const integral_type *>(ptr)) {} \
			TXScopeTaggedFixedConstPointer(const base_class & ptr) : base_class(ptr) {} \
			TXScopeTaggedFixedConstPointer<integral_type, TagID>& operator=(const TXScopeTaggedFixedConstPointer<integral_type, TagID> & _Right_cref) = delete; \
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION; \
			template<class integral_type2, size_t TagID2, class _TMemberObjectPointer> \
			friend auto make_xscope_tagged_pointer_to_member_v2(const TXScopeTaggedFixedConstPointer<integral_type2, TagID2> & lease_pointer, const _TMemberObjectPointer & member_object_ptr) \
				->TXScopeTaggedFixedConstPointer<typename std::remove_const<typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type>::type, TagID2>; \
			template<class integral_type2, size_t TagID2, class _TMemberObjectPointer> \
			friend auto make_xscope_tagged_const_pointer_to_member_v2(const TXScopeTaggedFixedPointer<integral_type2, TagID2> & lease_pointer, const _TMemberObjectPointer & member_object_ptr) \
				->TXScopeTaggedFixedConstPointer<typename std::remove_const<typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type>::type, TagID2>; \
			template<class integral_type2, size_t TagID2, class _TMemberObjectPointer> \
			friend auto make_xscope_tagged_const_pointer_to_member_v2(const TXScopeTaggedFixedConstPointer<integral_type2, TagID2> & lease_pointer, const _TMemberObjectPointer & member_object_ptr) \
				->TXScopeTaggedFixedConstPointer<typename std::remove_const<typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type>::type, TagID2>; \
		};

#define MSE_SCOPE_TAGGED_IMPL_INTEGRAL_SPECIALIZATION(integral_type) \
		MSE_SCOPE_TAGGED_IMPL_PTR_INTEGRAL_SPECIALIZATION(integral_type); \
		MSE_SCOPE_TAGGED_IMPL_OBJ_INTEGRAL_SPECIALIZATION(integral_type); \
		MSE_SCOPE_TAGGED_IMPL_PTR_INTEGRAL_SPECIALIZATION(typename std::add_const<integral_type>::type); \
		MSE_SCOPE_TAGGED_IMPL_OBJ_INTEGRAL_SPECIALIZATION(typename std::add_const<integral_type>::type);

		//MSE_SCOPE_TAGGED_IMPL_INTEGRAL_SPECIALIZATION(int);
		//MSE_SCOPE_TAGGED_IMPL_INTEGRAL_SPECIALIZATION(size_t);

#endif /*MSEPRIMITIVES_H*/

		/* end of template specializations */

#endif // 0


#endif /*MSE_SCOPEPOINTER_DISABLED*/

		template<typename _Ty, size_t TagID> using TXScopeTaggedItemFixedPointer = TXScopeTaggedFixedPointer<_Ty, TagID>;
		template<typename _Ty, size_t TagID> using TXScopeTaggedItemFixedConstPointer = TXScopeTaggedFixedConstPointer<_Ty, TagID>;

		namespace impl {

			template<typename TPtr, typename TTarget, size_t TagID>
			struct is_convertible_to_nonowning_scope_tagged_pointer_helper1 : std::integral_constant<bool,
				std::is_convertible<TPtr, mse::rsv::TXScopeTaggedFixedPointer<TTarget, TagID>>::value || std::is_convertible<TPtr, mse::rsv::TXScopeTaggedFixedConstPointer<TTarget, TagID>>::value> {};
			template<typename TPtr, size_t TagID>
			struct is_convertible_to_nonowning_scope_tagged_pointer : is_convertible_to_nonowning_scope_tagged_pointer_helper1<TPtr
				, typename std::remove_reference<decltype(*std::declval<TPtr>())>::type, TagID> {};

			template<typename TPtr, typename TTarget, size_t TagID>
			struct is_convertible_to_nonowning_scope_tagged_or_indeterminate_pointer_helper1 : std::integral_constant<bool,
				is_convertible_to_nonowning_scope_tagged_pointer<TPtr, TagID>::value
#ifdef MSE_SCOPEPOINTER_DISABLED
				|| std::is_convertible<TPtr, TTarget*>::value
#endif // MSE_SCOPEPOINTER_DISABLED
			> {};
			template<typename TPtr, size_t TagID>
			struct is_convertible_to_nonowning_scope_tagged_or_indeterminate_pointer : is_convertible_to_nonowning_scope_tagged_or_indeterminate_pointer_helper1
				<TPtr, typename std::remove_reference<decltype(*std::declval<TPtr>())>::type, TagID> {};

		}

#ifdef MSE_MSTDARRAY_DISABLED

		/* When mstd::array is "disabled" it is just aliased to std::array. But since std::array (and std::vector, etc.)
		iterators are dependent types, they do not participate in overload resolution. So the xscope_tagged_pointer() overload for
		those iterators actually needs to be a "universal" (template) overload that accepts any type. The reason it needs
		to be here (rather than in the msemstdarray.h file) is that if scope pointers are disabled, then it's possible that
		both scope pointers and std::array iterators could manifest as raw pointers and so would need to be handled (and
		(heuristically) disambiguated) by the same overload implementation. */

		namespace impl {

			template<class T, class EqualTo>
			struct HasOrInheritsIteratorCategoryMemberType_impl
			{
				template<class U, class V>
				static auto test(U*) -> decltype(std::declval<typename U::iterator_category>(), std::declval<typename V::iterator_category>(), bool(true));
				template<typename, typename>
				static auto test(...)->std::false_type;

				using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
			};
			template<class T, class EqualTo = T>
			struct HasOrInheritsIteratorCategoryMemberType : HasOrInheritsIteratorCategoryMemberType_impl<
				typename std::remove_reference<T>::type, typename std::remove_reference<EqualTo>::type>::type {};
			template<class _Ty>
			struct is_non_pointer_iterator : std::integral_constant<bool, HasOrInheritsIteratorCategoryMemberType<_Ty>::value> {};

		}
		template<size_t TagID, class _Ty>
		auto xscope_tagged_const_pointer(const _Ty& param) {
			return mse::rsv::us::unsafe_make_xscope_tagged_const_pointer_to(*param);
		}
		template<size_t TagID, class _Ty>
		auto xscope_tagged_pointer(const _Ty& param) {
			return mse::rsv::us::unsafe_make_xscope_tagged_pointer_to(*param);
		}
#endif // MSE_MSTDARRAY_DISABLED

#if defined(MSE_SCOPEPOINTER_DISABLED) && defined(MSE_MSTDARRAY_DISABLED)
#define MSE_SCOPE_TAGGED_POINTERS_AND_ITERATORS_MAY_BOTH_BE_RAW_POINTERS
#endif // defined(MSE_SCOPEPOINTER_DISABLED) && defined(MSE_MSTDARRAY_DISABLED)

#ifndef MSE_SCOPE_TAGGED_POINTERS_AND_ITERATORS_MAY_BOTH_BE_RAW_POINTERS
		/* When mstd::arrays, etc. are disabled, a "universal" overload of xscope_tagged_pointer() is provided for their iterators.
		That overload already handles raw pointers (which may be potentially ambiguous in that situation), so we shouldn't
		provide another one. */
		template <size_t TagID, typename _Ty>
		auto xscope_tagged_pointer(const mse::rsv::TXScopeTaggedFixedPointer<_Ty, TagID>& xsptr) {
			return xsptr;
		}
		template <size_t TagID, typename _Ty>
		auto xscope_tagged_const_pointer(const mse::rsv::TXScopeTaggedFixedPointer<_Ty, TagID>& xsptr) {
			return xsptr;
		}
		template <size_t TagID, typename _Ty>
		auto xscope_tagged_pointer(const mse::rsv::TXScopeTaggedFixedConstPointer<_Ty, TagID>& xsptr) {
			return xsptr;
		}
		template <size_t TagID, typename _Ty>
		auto xscope_tagged_const_pointer(const mse::rsv::TXScopeTaggedFixedConstPointer<_Ty, TagID>& xsptr) {
			return xsptr;
		}
#ifndef MSE_SCOPEPOINTER_DISABLED
		template <size_t TagID, typename _Ty>
		auto xscope_tagged_pointer(const mse::rsv::TXScopeTaggedObjFixedPointer<_Ty, TagID>& xsptr) {
			return xsptr;
		}
		template <size_t TagID, typename _Ty>
		auto xscope_tagged_const_pointer(const mse::rsv::TXScopeTaggedObjFixedPointer<_Ty, TagID>& xsptr) {
			return xsptr;
		}
		template <size_t TagID, typename _Ty>
		auto xscope_tagged_pointer(const mse::rsv::TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& xsptr) {
			return xsptr;
		}
		template <size_t TagID, typename _Ty>
		auto xscope_tagged_const_pointer(const mse::rsv::TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& xsptr) {
			return xsptr;
		}
#endif //!MSE_SCOPEPOINTER_DISABLED
#else // !MSE_SCOPE_TAGGED_POINTERS_AND_ITERATORS_MAY_BOTH_BE_RAW_POINTERS

#endif // !MSE_SCOPE_TAGGED_POINTERS_AND_ITERATORS_MAY_BOTH_BE_RAW_POINTERS

		namespace us {
			template <class _TTargetType, class _TLeaseType, size_t TagID> class TXScopeTaggedStrongFixedConstPointer;

			template <class _TTargetType, class _TLeaseType, size_t TagID>
			class TXScopeTaggedStrongFixedPointer : public mse::us::TStrongFixedPointer<_TTargetType, _TLeaseType>, public mse::us::impl::XScopeTaggedTagBase {
			public:
				typedef mse::us::TStrongFixedPointer<_TTargetType, _TLeaseType> base_class;

				TXScopeTaggedStrongFixedPointer(const TXScopeTaggedStrongFixedPointer&) = default;
				template<class _TLeaseType2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_TLeaseType2, _TLeaseType>::value, void>::type>
				TXScopeTaggedStrongFixedPointer(const TXScopeTaggedStrongFixedPointer<_TTargetType, _TLeaseType2, TagID2>& src_cref) : base_class(static_cast<const mse::us::TStrongFixedPointer<_TTargetType, _TLeaseType2>&>(src_cref)) {}

				template <class _TTargetType2, class _TLeaseType2>
				static TXScopeTaggedStrongFixedPointer make(_TTargetType2& target, const _TLeaseType2& lease) {
					return base_class::make(target, lease);
				}
				template <class _TTargetType2, class _TLeaseType2>
				static TXScopeTaggedStrongFixedPointer make(_TTargetType2& target, _TLeaseType2&& lease) {
					return base_class::make(target, std::forward<decltype(lease)>(lease));
				}

				auto xscope_tagged_ptr() const& {
					return mse::rsv::us::unsafe_make_xscope_tagged_pointer_to<TagID>(*(*this));
				}
				auto xscope_tagged_ptr() const&& = delete;
				operator mse::rsv::TXScopeTaggedFixedPointer<_TTargetType, TagID>() const& {
					return xscope_tagged_ptr();
				}
				operator mse::rsv::TXScopeTaggedFixedPointer<_TTargetType, TagID>() const&& = delete;

			protected:
				TXScopeTaggedStrongFixedPointer(_TTargetType& target/* often a struct member */, const _TLeaseType& lease/* usually a reference counting pointer */)
					: base_class(target, lease) {}
				TXScopeTaggedStrongFixedPointer(_TTargetType& target/* often a struct member */, _TLeaseType&& lease)
					: base_class(target, std::forward<decltype(lease)>(lease)) {}
			private:
				TXScopeTaggedStrongFixedPointer(const base_class& src_cref) : base_class(src_cref) {}

				TXScopeTaggedStrongFixedPointer& operator=(const TXScopeTaggedStrongFixedPointer& _Right_cref) = delete;
				MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;

				friend class TXScopeTaggedStrongFixedConstPointer<_TTargetType, _TLeaseType, TagID>;
			};

			template <class _TTargetType, class _TLeaseType, size_t TagID>
			TXScopeTaggedStrongFixedPointer<_TTargetType, _TLeaseType, TagID> make_xscope_tagged_strong(_TTargetType& target, const _TLeaseType& lease) {
				return TXScopeTaggedStrongFixedPointer<_TTargetType, _TLeaseType, TagID>::make(target, lease);
			}
			template <class _TTargetType, class _TLeaseType, size_t TagID>
			auto make_xscope_tagged_strong(_TTargetType& target, _TLeaseType&& lease) -> TXScopeTaggedStrongFixedPointer<_TTargetType, typename std::remove_reference<_TLeaseType>::type, TagID> {
				return TXScopeTaggedStrongFixedPointer<_TTargetType, typename std::remove_reference<_TLeaseType>::type, TagID>::make(target, std::forward<decltype(lease)>(lease));
			}

			template <class _TTargetType, class _TLeaseType, size_t TagID>
			class TXScopeTaggedStrongFixedConstPointer : public mse::us::TStrongFixedConstPointer<_TTargetType, _TLeaseType>, public mse::us::impl::XScopeTaggedTagBase {
			public:
				typedef mse::us::TStrongFixedConstPointer<_TTargetType, _TLeaseType> base_class;

				TXScopeTaggedStrongFixedConstPointer(const TXScopeTaggedStrongFixedConstPointer&) = default;
				template<class _TLeaseType2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_TLeaseType2, _TLeaseType>::value, void>::type>
				TXScopeTaggedStrongFixedConstPointer(const TXScopeTaggedStrongFixedConstPointer<_TTargetType, _TLeaseType2, TagID2>& src_cref) : base_class(static_cast<const mse::us::TStrongFixedConstPointer<_TTargetType, _TLeaseType2>&>(src_cref)) {}
				TXScopeTaggedStrongFixedConstPointer(const TXScopeTaggedStrongFixedPointer<_TTargetType, _TLeaseType, TagID>& src_cref) : base_class(static_cast<const mse::us::TStrongFixedConstPointer<_TTargetType, _TLeaseType>&>(src_cref)) {}
				template<class _TLeaseType2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_TLeaseType2, _TLeaseType>::value, void>::type>
				TXScopeTaggedStrongFixedConstPointer(const TXScopeTaggedStrongFixedPointer<_TTargetType, _TLeaseType2, TagID2>& src_cref) : base_class(static_cast<const mse::us::TStrongFixedConstPointer<_TTargetType, _TLeaseType2>&>(src_cref)) {}

				template <class _TTargetType2, class _TLeaseType2>
				static TXScopeTaggedStrongFixedConstPointer make(const _TTargetType2& target, const _TLeaseType2& lease) {
					return base_class::make(target, lease);
				}
				template <class _TTargetType2, class _TLeaseType2>
				static TXScopeTaggedStrongFixedConstPointer make(const _TTargetType2& target, _TLeaseType2&& lease) {
					return base_class::make(target, std::forward<decltype(lease)>(lease));
				}

				auto xscope_tagged_ptr() const& {
					return mse::rsv::us::unsafe_make_xscope_tagged_const_pointer_to<TagID>(*(*this));
				}
				auto xscope_tagged_ptr() const&& = delete;
				operator mse::rsv::TXScopeTaggedFixedConstPointer<_TTargetType, TagID>() const& {
					return xscope_tagged_ptr();
				}
				operator mse::rsv::TXScopeTaggedFixedConstPointer<_TTargetType, TagID>() const&& = delete;

			protected:
				TXScopeTaggedStrongFixedConstPointer(const _TTargetType& target/* often a struct member */, const _TLeaseType& lease/* usually a reference counting pointer */)
					: base_class(target, lease) {}
				TXScopeTaggedStrongFixedConstPointer(const _TTargetType& target/* often a struct member */, _TLeaseType&& lease)
					: base_class(target, std::forward<decltype(lease)>(lease)) {}
			private:
				TXScopeTaggedStrongFixedConstPointer(const base_class& src_cref) : base_class(src_cref) {}

				TXScopeTaggedStrongFixedConstPointer& operator=(const TXScopeTaggedStrongFixedConstPointer& _Right_cref) = delete;
				MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;
			};

			template <size_t TagID, class _TTargetType, class _TLeaseType>
			TXScopeTaggedStrongFixedConstPointer<_TTargetType, _TLeaseType, TagID> make_xscope_tagged_const_strong(const _TTargetType& target, const _TLeaseType& lease) {
				return TXScopeTaggedStrongFixedConstPointer<_TTargetType, _TLeaseType, TagID>::make(target, lease);
			}
			template <size_t TagID, class _TTargetType, class _TLeaseType>
			auto make_xscope_tagged_const_strong(const _TTargetType& target, _TLeaseType&& lease) -> TXScopeTaggedStrongFixedConstPointer<_TTargetType, typename std::remove_reference<_TLeaseType>::type, TagID> {
				return TXScopeTaggedStrongFixedConstPointer<_TTargetType, typename std::remove_reference<_TLeaseType>::type, TagID>::make(target, std::forward<decltype(lease)>(lease));
			}
		}


		namespace impl {
			template<size_t TagID, typename _Ty, class... Args>
			auto make_xscope_tagged_helper(std::true_type, Args&&... args) {
				return _Ty(std::forward<Args>(args)...);
			}
			template<size_t TagID, typename _Ty, class... Args>
			auto make_xscope_tagged_helper(std::false_type, Args&&... args) {
				return TXScopeTaggedObj<_Ty, TagID>(std::forward<Args>(args)...);
			}
		}
		template <size_t TagID, class X, class... Args>
		auto make_xscope_tagged(Args&&... args) {
			typedef typename std::remove_reference<X>::type nrX;
			//return impl::make_xscope_tagged_helper<TagID, nrX>(typename mse::impl::is_instantiation_of<nrX, TXScopeTaggedObj>::type(), std::forward<Args>(args)...);
			return impl::make_xscope_tagged_helper<TagID, nrX>(std::false_type(), std::forward<Args>(args)...);
		}
		template <size_t TagID, class X>
		auto make_xscope_tagged(const X& arg) {
			typedef typename std::remove_reference<X>::type nrX;
			//return impl::make_xscope_tagged_helper<TagID, nrX>(typename mse::impl::is_instantiation_of<nrX, TXScopeTaggedObj>::type(), arg);
			return impl::make_xscope_tagged_helper<TagID, nrX>(std::false_type(), arg);
		}
		template <size_t TagID, class X>
		auto make_xscope_tagged(X&& arg) {
			typedef typename std::remove_reference<X>::type nrX;
			//return impl::make_xscope_tagged_helper<TagID, nrX>(typename mse::impl::is_instantiation_of<nrX, TXScopeTaggedObj>::type(), std::forward<decltype(arg)>(arg));
			return impl::make_xscope_tagged_helper<TagID, nrX>(std::false_type(), std::forward<decltype(arg)>(arg));
		}

		namespace us {
			namespace impl {
#ifdef MSE_SCOPEPOINTER_DISABLED
				template<typename _Ty, size_t TagID> using TXScopeTaggedFixedPointerFParamBase = mse::us::impl::TPointer<_Ty>;
				template<typename _Ty, size_t TagID> using TXScopeTaggedFixedConstPointerFParamBase = mse::us::impl::TPointer<const _Ty>;
#else /*MSE_SCOPEPOINTER_DISABLED*/
				template<typename _Ty, size_t TagID> using TXScopeTaggedFixedPointerFParamBase = TXScopeTaggedFixedPointer<_Ty, TagID>;
				template<typename _Ty, size_t TagID> using TXScopeTaggedFixedConstPointerFParamBase = TXScopeTaggedFixedConstPointer<_Ty, TagID>;
#endif /*MSE_SCOPEPOINTER_DISABLED*/
			}
		}
	}

	namespace rsv {

		/* TXScopeTaggedFixedPointerFParam<> is just a version of TXScopeTaggedFixedPointer<> which may only be used for
		function parameter declations. It has the extra ability to accept (caged) scope pointers to r-value scope objects
		(i.e. supports temporaries by scope reference). */

		template<typename _Ty, size_t TagID>
		class TXScopeTaggedFixedPointerFParam : public mse::rsv::us::impl::TXScopeTaggedFixedPointerFParamBase<_Ty, TagID> {
		public:
			typedef mse::rsv::us::impl::TXScopeTaggedFixedPointerFParamBase<_Ty, TagID> base_class;
			MSE_SCOPE_TAGGED_USING(TXScopeTaggedFixedPointerFParam, base_class);

			TXScopeTaggedFixedPointerFParam(const TXScopeTaggedFixedPointerFParam& src_cref) = default;

#ifndef MSE_SCOPEPOINTER_DISABLED
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2 *, _Ty *>::value, void>::type>
			TXScopeTaggedFixedPointerFParam(TXScopeTaggedCagedItemFixedPointerToRValue<_Ty2, TagID>&& src_cref) : base_class(src_cref.uncaged_pointer()) {}

#ifndef MSE_TXSCOPETAGGEDCAGEDITEMFIXEDCONSTPOINTER_LEGACY_COMPATIBILITY1
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedPointerFParam(const TXScopeTaggedCagedItemFixedPointerToRValue<_Ty2, TagID>& src_cref) : base_class(src_cref.uncaged_pointer()) { intentional_compile_error<_Ty2>(); }
#else // !MSE_TXSCOPETAGGEDCAGEDITEMFIXEDCONSTPOINTER_LEGACY_COMPATIBILITY1
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedPointerFParam(const TXScopeTaggedCagedItemFixedPointerToRValue<_Ty2, TagID>& src_cref) : base_class(src_cref.uncaged_pointer()) {}
#endif // !MSE_TXSCOPETAGGEDCAGEDITEMFIXEDCONSTPOINTER_LEGACY_COMPATIBILITY1

#endif //!MSE_SCOPEPOINTER_DISABLED

			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedFixedPointerFParam() {}

			void xscope_tagged_not_returnable_tag() const {}
			void xscope_tagged_tag() const {}

		private:
			template<class _Ty2>
			void intentional_compile_error() const {
				/*
				You are attempting to use an lvalue "scope pointer to a temporary". (Currently) only rvalue
				"scope pointer to a temporary"s are supported.
				*/
				mse::impl::T_valid_if_same_msepointerbasics<_Ty2, void>();
			}
			TXScopeTaggedFixedPointerFParam<_Ty, TagID>& operator=(const TXScopeTaggedFixedPointerFParam<_Ty, TagID>& _Right_cref) = delete;
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;
		};

		template<typename _Ty, size_t TagID>
		class TXScopeTaggedFixedConstPointerFParam : public mse::rsv::us::impl::TXScopeTaggedFixedConstPointerFParamBase<_Ty, TagID> {
		public:
			typedef mse::rsv::us::impl::TXScopeTaggedFixedConstPointerFParamBase<_Ty, TagID> base_class;
			MSE_SCOPE_TAGGED_USING(TXScopeTaggedFixedConstPointerFParam, base_class);

			TXScopeTaggedFixedConstPointerFParam(const TXScopeTaggedFixedConstPointerFParam<_Ty, TagID>& src_cref) = default;
			TXScopeTaggedFixedConstPointerFParam(const TXScopeTaggedFixedPointerFParam<_Ty, TagID>& src_cref) : base_class(src_cref) {}

#ifndef MSE_SCOPEPOINTER_DISABLED
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedConstPointerFParam(TXScopeTaggedCagedItemFixedConstPointerToRValue<_Ty2, TagID>&& src_cref) : base_class(src_cref.uncaged_pointer()) {}
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedConstPointerFParam(TXScopeTaggedCagedItemFixedPointerToRValue<_Ty2, TagID>&& src_cref) : base_class(src_cref.uncaged_pointer()) {}

#ifndef MSE_TXSCOPETAGGEDCAGEDITEMFIXEDCONSTPOINTER_LEGACY_COMPATIBILITY1
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedConstPointerFParam(const TXScopeTaggedCagedItemFixedConstPointerToRValue<_Ty2, TagID>& src_cref) : base_class(src_cref.uncaged_pointer()) { intentional_compile_error<_Ty2>(); }
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedConstPointerFParam(const TXScopeTaggedCagedItemFixedPointerToRValue<_Ty2, TagID>& src_cref) : base_class(src_cref.uncaged_pointer()) { intentional_compile_error<_Ty2>(); }
#else // !MSE_TXSCOPETAGGEDCAGEDITEMFIXEDCONSTPOINTER_LEGACY_COMPATIBILITY1
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedConstPointerFParam(const TXScopeTaggedCagedItemFixedConstPointerToRValue<_Ty2, TagID>& src_cref) : base_class(src_cref.uncaged_pointer()) {}
			template<class _Ty2, size_t TagID2, class = typename std::enable_if<std::is_convertible<_Ty2*, _Ty*>::value, void>::type>
			TXScopeTaggedFixedConstPointerFParam(const TXScopeTaggedCagedItemFixedPointerToRValue<_Ty2, TagID>& src_cref) : base_class(src_cref.uncaged_pointer()) {}
#endif // !MSE_TXSCOPETAGGEDCAGEDITEMFIXEDCONSTPOINTER_LEGACY_COMPATIBILITY1

#endif //!MSE_SCOPEPOINTER_DISABLED

			MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedFixedConstPointerFParam() {}

			void xscope_tagged_not_returnable_tag() const {}
			void xscope_tagged_tag() const {}

		private:
			template<class _Ty2>
			void intentional_compile_error() const {
				/*
				You are attempting to use an lvalue "scope pointer to a temporary". (Currently) only rvalue 
				"scope pointer to a temporary"s are supported.
				*/
				mse::impl::T_valid_if_same_msepointerbasics<_Ty2, void>();
			}
			TXScopeTaggedFixedConstPointerFParam<_Ty, TagID>& operator=(const TXScopeTaggedFixedConstPointerFParam<_Ty, TagID>& _Right_cref) = delete;
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;
		};
	}

	namespace rsv {

		/* Template specializations of TFParam<>. There are a number of them. */

#ifndef MSE_SCOPEPOINTER_DISABLED
		template<typename _Ty, size_t TagID>
		class TFParam<mse::rsv::TXScopeTaggedFixedConstPointer<_Ty, TagID> > : public TXScopeTaggedFixedConstPointerFParam<_Ty, TagID> {
		public:
			typedef TXScopeTaggedFixedConstPointerFParam<_Ty, TagID> base_class;
			MSE_USING_AND_DEFAULT_COPY_AND_MOVE_CONSTRUCTOR_DECLARATIONS(TFParam, base_class);
			void xscope_tagged_not_returnable_tag() const {}
			void xscope_tagged_tag() const {}
		private:
			MSE_USING_ASSIGNMENT_OPERATOR_AND_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION(base_class);
		};

		template<typename _Ty, size_t TagID>
		class TFParam<const mse::rsv::TXScopeTaggedFixedConstPointer<_Ty, TagID> > : public TXScopeTaggedFixedConstPointerFParam<_Ty, TagID> {
		public:
			typedef TXScopeTaggedFixedConstPointerFParam<_Ty, TagID> base_class;
			MSE_USING_AND_DEFAULT_COPY_AND_MOVE_CONSTRUCTOR_DECLARATIONS(TFParam, base_class);
			void xscope_tagged_not_returnable_tag() const {}
			void xscope_tagged_tag() const {}
		private:
			MSE_USING_ASSIGNMENT_OPERATOR_AND_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION(base_class);
		};

		template<typename _Ty, size_t TagID>
		class TFParam<mse::rsv::TXScopeTaggedCagedItemFixedConstPointerToRValue<_Ty, TagID> > : public TXScopeTaggedFixedConstPointerFParam<_Ty, TagID> {
		public:
			typedef TXScopeTaggedFixedConstPointerFParam<_Ty, TagID> base_class;
			MSE_USING_AND_DEFAULT_COPY_AND_MOVE_CONSTRUCTOR_DECLARATIONS(TFParam, base_class);
			void xscope_tagged_not_returnable_tag() const {}
			void xscope_tagged_tag() const {}
		private:
			MSE_USING_ASSIGNMENT_OPERATOR_AND_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION(base_class);
		};
#endif //!MSE_SCOPEPOINTER_DISABLED

	}

	namespace rsv {
		/* If a rsv::TReturnableFParam<> wrapped reference is used to make a pointer to a member of its target object, then the
	created pointer to member can inherit the "returnability" of the original wrapped reference. */
	/* Overloads for rsv::TReturnableFParam<>. */
		MSE_OVERLOAD_FOR_RETURNABLE_FPARAM_DECLARATION(make_xscope_tagged_pointer_to_member_v2)
			MSE_OVERLOAD_FOR_RETURNABLE_FPARAM_DECLARATION(make_xscope_tagged_const_pointer_to_member_v2)

			MSE_OVERLOAD_FOR_RETURNABLE_FPARAM_DECLARATION(xscope_tagged_pointer)

			template<typename _TROy>
		class TXScopeTaggedReturnValue : public TReturnValue<_TROy>
			, public MSE_FIRST_OR_PLACEHOLDER_IF_A_BASE_OF_SECOND(mse::us::impl::XScopeTaggedTagBase, TReturnValue<_TROy>, TXScopeTaggedReturnValue<_TROy>)
		{
		public:
			typedef TReturnValue<_TROy> base_class;
			MSE_USING_AND_DEFAULT_COPY_AND_MOVE_CONSTRUCTOR_DECLARATIONS_AND_USING_ASSIGNMENT_OPERATOR(TXScopeTaggedReturnValue, base_class);

			template<class _Ty2 = _TROy, class = typename std::enable_if<(std::is_same<_Ty2, _TROy>::value) && (mse::impl::is_potentially_not_xscope<_Ty2>::value), void>::type>
			void xscope_tagged_returnable_tag() const {} /* Indication that this type is eligible to be used as a function return value. */

		private:
			MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;
		};


		/* TXScopeTaggedOwnerPointer is meant to be much like boost::scoped_ptr<>. Instead of taking a native pointer,
		TXScopeTaggedOwnerPointer just forwards it's constructor arguments to the constructor of the TXScopeTaggedObj<_Ty, TagID>.
		TXScopeTaggedOwnerPointers are meant to be allocated on the stack only. Unfortunately there's really no way to
		enforce this, which makes this data type less intrinsically safe than say, "reference counting" pointers.
		*/
		template<typename _Ty, size_t TagID>
		class TXScopeTaggedOwnerPointer : public mse::us::impl::XScopeTaggedTagBase, public mse::us::impl::StrongPointerAsyncNotShareableAndNotPassableTagBase
			, mse::us::impl::ReferenceableByScopeTaggedPointerTagBase
			//, MSE_FIRST_OR_PLACEHOLDER_IF_NOT_A_BASE_OF_SECOND(mse::us::impl::ContainsNonOwningScopeTaggedReferenceTagBase, _Ty, TXScopeTaggedOwnerPointer<_Ty, TagID>)
		{
		public:
			TXScopeTaggedOwnerPointer(TXScopeTaggedOwnerPointer<_Ty, TagID>&& src_ref) = default;

			template <class... Args>
			TXScopeTaggedOwnerPointer(Args&&... args) {
				/* In the case where there is exactly one argument and its type is derived from this type, we want to
				act like a move constructor here. We use a helper function to check for this case and act accordingly. */
				constructor_helper1(std::forward<Args>(args)...);
			}

			TXScopeTaggedObj<_Ty, TagID>& operator*() const& {
				return (*m_ptr);
			}
			TXScopeTaggedObj<_Ty, TagID>&& operator*() const&& {
				return std::move(*m_ptr);
			}
			TXScopeTaggedObj<_Ty, TagID>* operator->() const& {
				return std::addressof(*m_ptr);
			}
			void operator->() const&& = delete;

#ifdef MSE_SCOPEPOINTER_DISABLED
			operator _Ty* () const {
				return std::addressof(*(*this));
			}
			explicit operator const _Ty* () const {
				return std::addressof(*(*this));
			}
#endif /*MSE_SCOPEPOINTER_DISABLED*/

			template <class... Args>
			static TXScopeTaggedOwnerPointer make(Args&&... args) {
				return TXScopeTaggedOwnerPointer(std::forward<Args>(args)...);
			}

			void xscope_tagged_tag() const {}
			/* This type can be safely used as a function return value if _TROy is also safely returnable. */
			template<class _Ty2 = _Ty, class = typename std::enable_if<(std::is_same<_Ty2, _Ty>::value) && (
				(std::integral_constant<bool, mse::impl::HasXScopeTaggedReturnableTagMethod<_Ty2>::Has>()) || (mse::impl::is_potentially_not_xscope<_Ty2>::value)
				), void>::type>
				void xscope_tagged_returnable_tag() const {} /* Indication that this type is can be used as a function return value. */

		private:
			/* construction helper functions */
			template <class... Args>
			void initialize(Args&&... args) {
				/* We can't use std::make_unique<> because TXScopeTaggedObj<>'s "operator new()" is private and inaccessible to
				std::make_unique<> (which is not a friend of TXScopeTaggedObj<> like we are). */
				auto new_ptr = new TXScopeTaggedObj<_Ty, TagID>(std::forward<Args>(args)...);
				m_ptr.reset(new_ptr);
			}
			template <class _TSoleArg>
			void constructor_helper2(std::true_type, _TSoleArg&& sole_arg) {
				/* The sole parameter is derived from, or of this type, so we're going to consider the constructor
				a move constructor. */
#ifdef MSE_RESTRICT_TXSCOPETAGGEDOWNERPOINTER_MOVABILITY
				/* You would probably only consider defining MSE_RESTRICT_TXSCOPETAGGEDOWNERPOINTER_MOVABILITY for extra safety if for
				some reason you couldn't rely on the availability of a tool (like scpptool) to statically enforce the safety of
				these moves. */
#ifdef MSE_HAS_CXX17
				static_assert(false, "The MSE_RESTRICT_TXSCOPETAGGEDOWNERPOINTER_MOVABILITY preprocessor symbol is defined, which prohibits the use of TXScopeTaggedOwnerPointer<>'s move constructor. ");
#endif // MSE_HAS_CXX17
#endif // MSE_RESTRICT_TXSCOPETAGGEDOWNERPOINTER_MOVABILITY
				m_ptr = std::forward<decltype(sole_arg.m_ptr)>(sole_arg.m_ptr);
			}
			template <class _TSoleArg>
			void constructor_helper2(std::false_type, _TSoleArg&& sole_arg) {
				/* The sole parameter is not derived from, or of this type, so the constructor is not a move
				constructor. */
				initialize(std::forward<decltype(sole_arg)>(sole_arg));
			}
			template <class... Args>
			void constructor_helper1(Args&&... args) {
				initialize(std::forward<Args>(args)...);
			}
			template <class _TSoleArg>
			void constructor_helper1(_TSoleArg&& sole_arg) {
				/* The constructor was given exactly one parameter. If the parameter is derived from, or of this type,
				then we're going to consider the constructor a move constructor. */

				constructor_helper2(typename std::is_base_of<TXScopeTaggedOwnerPointer, _TSoleArg>::type(), std::forward<decltype(sole_arg)>(sole_arg));
			}

			TXScopeTaggedOwnerPointer(const TXScopeTaggedOwnerPointer<_Ty, TagID>& src_cref) = delete;
			TXScopeTaggedOwnerPointer<_Ty, TagID>& operator=(const TXScopeTaggedOwnerPointer<_Ty, TagID>& _Right_cref) = delete;
			void* operator new(size_t size) { return ::operator new(size); }

			std::unique_ptr<TXScopeTaggedObj<_Ty, TagID> > m_ptr = nullptr;
		};

		template <class _Ty2, size_t TagID, class... Args>
		static TXScopeTaggedOwnerPointer<_Ty2, TagID> make_xscope_tagged_owner(Args&&... args) {
			return TXScopeTaggedOwnerPointer<_Ty2, TagID>::make(std::forward<Args>(args)...);
		}
	}
}

namespace std {
	template<class _Ty, size_t TagID>
	struct hash<mse::rsv::TXScopeTaggedOwnerPointer<_Ty, TagID> > {	// hash functor
		typedef mse::rsv::TXScopeTaggedOwnerPointer<_Ty, TagID> argument_type;
		typedef size_t result_type;
		size_t operator()(const mse::rsv::TXScopeTaggedOwnerPointer<_Ty, TagID>& _Keyval) const _NOEXCEPT {
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
		namespace us {
			/* (Unsafely) obtain a scope pointer to any object. */
			template<size_t TagID, typename _Ty>
			TXScopeTaggedFixedPointer<_Ty, TagID> unsafe_make_xscope_tagged_pointer_to(_Ty& ref) {
				return TXScopeTaggedFixedPointer<_Ty, TagID>(std::addressof(ref));
			}
			template<size_t TagID, typename _Ty>
			TXScopeTaggedFixedConstPointer<_Ty, TagID> unsafe_make_xscope_tagged_const_pointer_to(const _Ty& cref) {
				return TXScopeTaggedFixedConstPointer<_Ty, TagID>(std::addressof(cref));
			}
			template<size_t TagID, typename _Ty>
			TXScopeTaggedFixedConstPointer<_Ty, TagID> unsafe_make_xscope_tagged_pointer_to(const _Ty& cref) {
				return unsafe_make_xscope_tagged_const_pointer_to(cref);
			}
		}
	}

	namespace rsv {
		/* Obtain a scope pointer to any object. Requires static verification. */
		template<size_t TagID, typename _Ty>
		TXScopeTaggedFixedPointer<_Ty, TagID> make_xscope_tagged_pointer_to(_Ty& ref) {
			return mse::rsv::us::unsafe_make_xscope_tagged_pointer_to(ref);
		}
		template<size_t TagID, typename _Ty>
		TXScopeTaggedFixedConstPointer<_Ty, TagID> make_xscope_tagged_const_pointer_to(const _Ty& cref) {
			return mse::rsv::us::unsafe_make_xscope_tagged_const_pointer_to(cref);
		}
		template<size_t TagID, typename _Ty>
		TXScopeTaggedFixedConstPointer<_Ty, TagID> make_xscope_tagged_pointer_to(const _Ty& cref) {
			return make_xscope_tagged_const_pointer_to(cref);
		}
	}

	namespace rsv {
#if 0
		namespace us {
			template<typename _TROy>
			class TXScopeTaggedUserDeclaredReturnable : public _TROy {
			public:
				MSE_USING(TXScopeTaggedUserDeclaredReturnable, _TROy);
				TXScopeTaggedUserDeclaredReturnable(const TXScopeTaggedUserDeclaredReturnable& _X) : _TROy(_X) {}
				TXScopeTaggedUserDeclaredReturnable(TXScopeTaggedUserDeclaredReturnable&& _X) : _TROy(std::forward<decltype(_X)>(_X)) {}
				MSE_IMPL_DESTRUCTOR_PREFIX1 ~TXScopeTaggedUserDeclaredReturnable() {
					/* This is just a no-op function that will cause a compile error when _TROy is a prohibited type. */
					valid_if_TROy_is_not_marked_as_unreturn_value();
					valid_if_TROy_is_an_xscope_tagged_type();
				}

				template<class _Ty2>
				TXScopeTaggedUserDeclaredReturnable& operator=(_Ty2&& _X) { _TROy::operator=(std::forward<decltype(_X)>(_X)); return (*this); }
				template<class _Ty2>
				TXScopeTaggedUserDeclaredReturnable& operator=(const _Ty2& _X) { _TROy::operator=(_X); return (*this); }

				void xscope_tagged_returnable_tag() const {} /* Indication that this type is eligible to be used as a function return value. */

			private:

				/* If _TROy is "marked" as not safe to use as a function return value, then the following member function
				will not instantiate, causing an (intended) compile error. */
				template<class = typename std::enable_if<(mse::impl::potentially_does_not_contain_non_owning_scope_tagged_reference<_TROy>::value)
					/*&& (!std::integral_constant<bool, mse::impl::HasXScopeTaggedNotReturnableTagMethod<_TROy>::Has>())*/, void>::type>
					void valid_if_TROy_is_not_marked_as_unreturn_value() const {}

				template<class = typename std::enable_if<mse::impl::is_potentially_xscope<_TROy>::value, void>::type>
				void valid_if_TROy_is_an_xscope_tagged_type() const {}

				MSE_DEFAULT_OPERATOR_NEW_AND_AMPERSAND_DECLARATION;
			};
		}
#endif // 0



#ifndef MSE_SCOPE_TAGGED_DISABLE_MAKE_POINTER_TO_MEMBER

#ifdef MSE_SCOPEPOINTER_DISABLED
		namespace impl {
			/* This template type alias is only used because msvc2017(v15.9.0) crashes if the type expression is used directly. */
			template<class _Ty2, size_t TagID2, class _TMemberObjectPointer>
			using make_xscope_tagged_pointer_to_member_v2_return_type1 = TXScopeTaggedFixedPointer<typename std::remove_reference<decltype(std::declval<_Ty2>().*std::declval<_TMemberObjectPointer>())>::type, TagID2>;
		}
#endif // MSE_SCOPEPOINTER_DISABLED

		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_xscope_tagged_pointer_to_member_v2(const TXScopeTaggedFixedPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr)
			-> mse::rsv::impl::make_xscope_tagged_pointer_to_member_v2_return_type1<_Ty, TagID, _TMemberObjectPointer> {
			mse::impl::make_pointer_to_member_v2_checks_msepointerbasics(lease_pointer, member_object_ptr);
			/* Originally, this function itself was a friend of TXScopeTaggedFixedPointer<>, but that seemed to confuse msvc2017 (but not
			g++ or clang) in some cases. */
			//typedef typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type _TTarget;
			//return TXScopeTaggedFixedPointer<_TTarget>(std::addressof((*lease_pointer).*member_object_ptr));
			return mse::rsv::us::unsafe_make_xscope_tagged_pointer_to<TagID>((*lease_pointer).*member_object_ptr);
		}
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_xscope_tagged_pointer_to_member_v2(const TXScopeTaggedFixedConstPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr)
			-> TXScopeTaggedFixedConstPointer<typename std::remove_const<typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type>::type, TagID> {
			mse::impl::make_pointer_to_member_v2_checks_msepointerbasics(lease_pointer, member_object_ptr);
			/* Originally, this function itself was a friend of TXScopeTaggedFixedConstPointer<>, but that seemed to confuse msvc2017 (but not
			g++ or clang) in some cases. */
			//typedef typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type _TTarget;
			//return TXScopeTaggedFixedConstPointer<_TTarget>(std::addressof((*lease_pointer).*member_object_ptr));
			return mse::rsv::us::unsafe_make_xscope_tagged_const_pointer_to<TagID>((*lease_pointer).*member_object_ptr);
		}
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_xscope_tagged_const_pointer_to_member_v2(const TXScopeTaggedFixedPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr)
			-> TXScopeTaggedFixedConstPointer<typename std::remove_const<typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type>::type, TagID> {
			mse::impl::make_pointer_to_member_v2_checks_msepointerbasics(lease_pointer, member_object_ptr);
			/* Originally, this function itself was a friend of TXScopeTaggedFixedConstPointer<>, but that seemed to confuse msvc2017 (but not
			g++ or clang) in some cases. */
			//typedef typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type _TTarget;
			//return TXScopeTaggedFixedConstPointer<_TTarget>(std::addressof((*lease_pointer).*member_object_ptr));
			return mse::rsv::us::unsafe_make_xscope_tagged_const_pointer_to<TagID>((*lease_pointer).*member_object_ptr);
		}
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_xscope_tagged_const_pointer_to_member_v2(const TXScopeTaggedFixedConstPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr)
			-> TXScopeTaggedFixedConstPointer<typename std::remove_const<typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type>::type, TagID> {
			mse::impl::make_pointer_to_member_v2_checks_msepointerbasics(lease_pointer, member_object_ptr);
			/* Originally, this function itself was a friend of TXScopeTaggedFixedConstPointer<>, but that seemed to confuse msvc2017 (but not
			g++ or clang) in some cases. */
			//typedef typename std::remove_reference<decltype((*lease_pointer).*member_object_ptr)>::type _TTarget;
			//return TXScopeTaggedFixedConstPointer<_TTarget>(std::addressof((*lease_pointer).*member_object_ptr));
			return mse::rsv::us::unsafe_make_xscope_tagged_const_pointer_to<TagID>((*lease_pointer).*member_object_ptr);
		}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_xscope_tagged_pointer_to_member_v2(const TXScopeTaggedObjFixedPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr) {
			return make_xscope_tagged_pointer_to_member_v2(TXScopeTaggedFixedPointer<_Ty, TagID>(lease_pointer), member_object_ptr);
		}
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_xscope_tagged_pointer_to_member_v2(const TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr) {
			return make_xscope_tagged_pointer_to_member_v2(TXScopeTaggedFixedConstPointer<_Ty, TagID>(lease_pointer), member_object_ptr);
		}
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_xscope_tagged_const_pointer_to_member_v2(const TXScopeTaggedObjFixedPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr) {
			return make_xscope_tagged_const_pointer_to_member_v2(TXScopeTaggedFixedPointer<_Ty, TagID>(lease_pointer), member_object_ptr);
		}
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_xscope_tagged_const_pointer_to_member_v2(const TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr) {
			return make_xscope_tagged_const_pointer_to_member_v2(TXScopeTaggedFixedConstPointer<_Ty, TagID>(lease_pointer), member_object_ptr);
		}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)

		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_pointer_to_member_v2(const TXScopeTaggedFixedPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr) {
			return make_xscope_tagged_pointer_to_member_v2(lease_pointer, member_object_ptr);
		}
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_pointer_to_member_v2(const TXScopeTaggedFixedConstPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr) {
			return make_xscope_tagged_pointer_to_member_v2(lease_pointer, member_object_ptr);
		}
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_const_pointer_to_member_v2(const TXScopeTaggedFixedPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr) {
			return make_xscope_tagged_const_pointer_to_member_v2(lease_pointer, member_object_ptr);
		}
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_const_pointer_to_member_v2(const TXScopeTaggedFixedConstPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr) {
			return make_xscope_tagged_const_pointer_to_member_v2(lease_pointer, member_object_ptr);
		}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_pointer_to_member_v2(const TXScopeTaggedObjFixedPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr) {
			return make_xscope_tagged_pointer_to_member_v2(lease_pointer, member_object_ptr);
		}
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_pointer_to_member_v2(const TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr) {
			return make_xscope_tagged_pointer_to_member_v2(lease_pointer, member_object_ptr);
		}
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_const_pointer_to_member_v2(const TXScopeTaggedObjFixedPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr) {
			return make_xscope_tagged_const_pointer_to_member_v2(lease_pointer, member_object_ptr);
		}
		template<class _Ty, size_t TagID, class _TMemberObjectPointer>
		auto make_const_pointer_to_member_v2(const TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& lease_pointer, const _TMemberObjectPointer& member_object_ptr) {
			return make_xscope_tagged_const_pointer_to_member_v2(lease_pointer, member_object_ptr);
		}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)

#endif // !MSE_SCOPE_TAGGED_DISABLE_MAKE_POINTER_TO_MEMBER


		/* TXScopeTaggedStrongPointerStore et al are types that store a strong pointer (like a refcounting pointer), and let you
		obtain a corresponding scope pointer. */
		template<typename _TStrongPointer, size_t TagID>
		class TXScopeTaggedStrongPointerStore : public mse::us::impl::XScopeTaggedTagBase
			//, public MSE_FIRST_OR_PLACEHOLDER_IF_NOT_A_BASE_OF_SECOND(mse::us::impl::ContainsNonOwningScopeTaggedReferenceTagBase, typename std::remove_reference<_TStrongPointer>::type, TXScopeTaggedStrongPointerStore<typename std::remove_reference<_TStrongPointer>::type, TagID>)
		{
		private:
			typedef typename std::remove_reference<_TStrongPointer>::type _TStrongPointerNR;
			_TStrongPointerNR m_stored_ptr;

		public:
			typedef TXScopeTaggedStrongPointerStore _Myt;
			typedef typename std::remove_reference<decltype(*m_stored_ptr)>::type target_t;

			TXScopeTaggedStrongPointerStore(const TXScopeTaggedStrongPointerStore&) = delete;
			TXScopeTaggedStrongPointerStore(TXScopeTaggedStrongPointerStore&&) = default;

			TXScopeTaggedStrongPointerStore(_TStrongPointerNR && stored_ptr) : m_stored_ptr(std::forward<_TStrongPointerNR>(stored_ptr)) {
				*m_stored_ptr; /* Just verifying that stored_ptr points to a valid target. */
			}
			TXScopeTaggedStrongPointerStore(const _TStrongPointerNR & stored_ptr) : m_stored_ptr(stored_ptr) {
				*stored_ptr; /* Just verifying that stored_ptr points to a valid target. */
			}
			~TXScopeTaggedStrongPointerStore() {
				mse::impl::is_valid_if_strong_pointer<_TStrongPointerNR>::no_op();
			}
			auto xscope_tagged_ptr() const& {
				return mse::rsv::us::unsafe_make_xscope_tagged_pointer_to<TagID>(*m_stored_ptr);
			}
			auto xscope_tagged_ptr() const&& {
				return mse::rsv::TXScopeTaggedCagedItemFixedPointerToRValue<target_t, TagID>(mse::rsv::us::unsafe_make_xscope_tagged_pointer_to<TagID>(*m_stored_ptr));
			}
			const _TStrongPointerNR& stored_ptr() const { return m_stored_ptr; }

			operator mse::rsv::TXScopeTaggedFixedPointer<target_t, TagID>() const& {
				return m_stored_ptr;
			}
			auto& operator*() const {
				return *m_stored_ptr;
			}
			auto* operator->() const {
				return std::addressof(*m_stored_ptr);
			}
			bool operator==(const _Myt & rhs) const {
				return (rhs.m_stored_ptr == m_stored_ptr);
			}

			void async_not_shareable_and_not_passable_tag() const {}
			/* This type can be safely used as a function return value if the element it contains is also safely returnable. */
			template<class _Ty2 = _TStrongPointerNR, class = typename std::enable_if<(std::is_same<_Ty2, _TStrongPointerNR>::value) && (
				(std::integral_constant<bool, mse::impl::HasXScopeTaggedReturnableTagMethod<_Ty2>::Has>()) || (mse::impl::is_potentially_not_xscope<_Ty2>::value)
				), void>::type>
				void xscope_tagged_returnable_tag() const {} /* Indication that this type is can be used as a function return value. */
		};

		template<typename _TStrongPointer, size_t TagID>
		class TXScopeTaggedStrongConstPointerStore : public mse::us::impl::XScopeTaggedTagBase
			//, public MSE_FIRST_OR_PLACEHOLDER_IF_NOT_A_BASE_OF_SECOND(mse::us::impl::ContainsNonOwningScopeTaggedReferenceTagBase, typename std::remove_reference<_TStrongPointer>::type, TXScopeTaggedStrongConstPointerStore<typename std::remove_reference<_TStrongPointer>::type, TagID>)
		{
		private:
			typedef typename std::remove_reference<_TStrongPointer>::type _TStrongPointerNR;
			_TStrongPointerNR m_stored_ptr;
			static void dummy_foo(const decltype(*std::declval<_TStrongPointerNR>())&) {}

		public:
			typedef TXScopeTaggedStrongConstPointerStore _Myt;
			typedef typename std::remove_reference<decltype(*m_stored_ptr)>::type target_t;

			TXScopeTaggedStrongConstPointerStore(const TXScopeTaggedStrongConstPointerStore&) = delete;
			TXScopeTaggedStrongConstPointerStore(TXScopeTaggedStrongConstPointerStore&&) = default;

			TXScopeTaggedStrongConstPointerStore(const _TStrongPointerNR & stored_ptr) : m_stored_ptr(stored_ptr) {
				dummy_foo(*stored_ptr); /* Just verifying that stored_ptr points to a valid target. */
			}
			~TXScopeTaggedStrongConstPointerStore() {
				mse::impl::is_valid_if_strong_pointer<_TStrongPointerNR>::no_op();
			}
			auto xscope_tagged_ptr() const& {
				return mse::rsv::us::unsafe_make_xscope_tagged_const_pointer_to<TagID>(*m_stored_ptr);
			}
			auto xscope_tagged_ptr() const&& {
				return mse::rsv::TXScopeTaggedCagedItemFixedConstPointerToRValue<typename std::remove_const<typename std::remove_reference<decltype(*m_stored_ptr)>::type>::type, TagID>(mse::rsv::us::unsafe_make_xscope_tagged_const_pointer_to<TagID>(*m_stored_ptr));
			}
			const _TStrongPointerNR& stored_ptr() const { return m_stored_ptr; }

			operator mse::rsv::TXScopeTaggedFixedConstPointer<target_t, TagID>() const& {
				return m_stored_ptr;
			}
			auto& operator*() const {
				return *m_stored_ptr;
			}
			auto* operator->() const {
				return std::addressof(*m_stored_ptr);
			}
			bool operator==(const _Myt & rhs) const {
				return (rhs.m_stored_ptr == m_stored_ptr);
			}

			void async_not_shareable_and_not_passable_tag() const {}
			/* This type can be safely used as a function return value if the element it contains is also safely returnable. */
			template<class _Ty2 = _TStrongPointerNR, class = typename std::enable_if<(std::is_same<_Ty2, _TStrongPointerNR>::value) && (
				(std::integral_constant<bool, mse::impl::HasXScopeTaggedReturnableTagMethod<_Ty2>::Has>()) || (mse::impl::is_potentially_not_xscope<_Ty2>::value)
				), void>::type>
				void xscope_tagged_returnable_tag() const {} /* Indication that this type is can be used as a function return value. */
		};

		template<typename _TStrongPointer, size_t TagID, class = mse::impl::is_valid_if_strong_and_never_null_pointer<typename std::remove_reference<_TStrongPointer>::type> >
		class TXScopeTaggedStrongNotNullPointerStore : public mse::us::impl::XScopeTaggedTagBase
			//, public MSE_FIRST_OR_PLACEHOLDER_IF_NOT_A_BASE_OF_SECOND(mse::us::impl::ContainsNonOwningScopeTaggedReferenceTagBase, typename std::remove_reference<_TStrongPointer>::type, TXScopeTaggedStrongNotNullPointerStore<typename std::remove_reference<_TStrongPointer>::type>, TagID)
		{
		private:
			typedef typename std::remove_reference<_TStrongPointer>::type _TStrongPointerNR;
			_TStrongPointerNR m_stored_ptr;

		public:
			typedef TXScopeTaggedStrongNotNullPointerStore _Myt;
			typedef typename std::remove_reference<decltype(*m_stored_ptr)>::type target_t;

			TXScopeTaggedStrongNotNullPointerStore(const TXScopeTaggedStrongNotNullPointerStore&) = delete;
			TXScopeTaggedStrongNotNullPointerStore(TXScopeTaggedStrongNotNullPointerStore&&) = default;

			TXScopeTaggedStrongNotNullPointerStore(const _TStrongPointerNR & stored_ptr) : m_stored_ptr(stored_ptr) {}
			auto xscope_tagged_ptr() const& {
				return mse::rsv::us::unsafe_make_xscope_tagged_pointer_to<TagID>(*m_stored_ptr);
			}
			void xscope_tagged_ptr() const&& = delete;
			const _TStrongPointerNR& stored_ptr() const { return m_stored_ptr; }

			operator mse::rsv::TXScopeTaggedFixedPointer<target_t, TagID>() const& {
				return m_stored_ptr;
			}
			auto& operator*() const {
				return *m_stored_ptr;
			}
			auto* operator->() const {
				return std::addressof(*m_stored_ptr);
			}
			bool operator==(const _Myt & rhs) const {
				return (rhs.m_stored_ptr == m_stored_ptr);
			}

			void async_not_shareable_and_not_passable_tag() const {}
			/* This type can be safely used as a function return value if the element it contains is also safely returnable. */
			template<class _Ty2 = _TStrongPointerNR, class = typename std::enable_if<(std::is_same<_Ty2, _TStrongPointerNR>::value) && (
				(std::integral_constant<bool, mse::impl::HasXScopeTaggedReturnableTagMethod<_Ty2>::Has>()) || (mse::impl::is_potentially_not_xscope<_Ty2>::value)
				), void>::type>
				void xscope_tagged_returnable_tag() const {} /* Indication that this type is can be used as a function return value. */
		};

		template<typename _TStrongPointer, size_t TagID, class = mse::impl::is_valid_if_strong_and_never_null_pointer<typename std::remove_reference<_TStrongPointer>::type> >
		class TXScopeTaggedStrongNotNullConstPointerStore : public mse::us::impl::XScopeTaggedTagBase
			//, public MSE_FIRST_OR_PLACEHOLDER_IF_NOT_A_BASE_OF_SECOND(mse::us::impl::ContainsNonOwningScopeTaggedReferenceTagBase, _TStrongPointer, TXScopeTaggedStrongNotNullConstPointerStore<typename std::remove_reference<_TStrongPointer>::type>, TagID)
		{
		private:
			typedef typename std::remove_reference<_TStrongPointer>::type _TStrongPointerNR;
			_TStrongPointerNR m_stored_ptr;

		public:
			typedef TXScopeTaggedStrongNotNullConstPointerStore _Myt;
			typedef typename std::remove_reference<decltype(*m_stored_ptr)>::type target_t;

			TXScopeTaggedStrongNotNullConstPointerStore(const TXScopeTaggedStrongNotNullConstPointerStore&) = delete;
			TXScopeTaggedStrongNotNullConstPointerStore(TXScopeTaggedStrongNotNullConstPointerStore&&) = default;

			TXScopeTaggedStrongNotNullConstPointerStore(const _TStrongPointerNR & stored_ptr) : m_stored_ptr(stored_ptr) {}
			auto xscope_tagged_ptr() const& {
				return mse::rsv::us::unsafe_make_xscope_tagged_const_pointer_to<TagID>(*m_stored_ptr);
			}
			void xscope_tagged_ptr() const&& = delete;
			const _TStrongPointerNR& stored_ptr() const { return m_stored_ptr; }

			operator mse::rsv::TXScopeTaggedFixedConstPointer<target_t, TagID>() const& {
				return m_stored_ptr;
			}
			auto& operator*() const {
				return *m_stored_ptr;
			}
			auto* operator->() const {
				return std::addressof(*m_stored_ptr);
			}
			bool operator==(const _Myt & rhs) const {
				return (rhs.m_stored_ptr == m_stored_ptr);
			}

			void async_not_shareable_and_not_passable_tag() const {}
			/* This type can be safely used as a function return value if the element it contains is also safely returnable. */
			template<class _Ty2 = _TStrongPointerNR, class = typename std::enable_if<(std::is_same<_Ty2, _TStrongPointerNR>::value) && (
				(std::integral_constant<bool, mse::impl::HasXScopeTaggedReturnableTagMethod<_Ty2>::Has>()) || (mse::impl::is_potentially_not_xscope<_Ty2>::value)
				), void>::type>
				void xscope_tagged_returnable_tag() const {} /* Indication that this type is can be used as a function return value. */
		};

		namespace impl {
			namespace ns_xscope_tagged_strong_pointer_store {

				template<size_t TagID, typename _TStrongPointer>
				auto make_xscope_tagged_strong_const_pointer_store_helper1(std::true_type, const _TStrongPointer& stored_ptr) {
					return TXScopeTaggedStrongNotNullConstPointerStore<_TStrongPointer, TagID>(stored_ptr);
				}
				template<size_t TagID, typename _TStrongPointer>
				auto make_xscope_tagged_strong_const_pointer_store_helper1(std::true_type, _TStrongPointer&& stored_ptr) {
					return TXScopeTaggedStrongNotNullConstPointerStore<_TStrongPointer, TagID>(std::forward<decltype(stored_ptr)>(stored_ptr));
				}
				template<size_t TagID, typename _TStrongPointer>
				auto make_xscope_tagged_strong_const_pointer_store_helper1(std::false_type, const _TStrongPointer& stored_ptr) {
					return TXScopeTaggedStrongConstPointerStore<_TStrongPointer, TagID>(stored_ptr);
				}
				template<size_t TagID, typename _TStrongPointer>
				auto make_xscope_tagged_strong_const_pointer_store_helper1(std::false_type, _TStrongPointer&& stored_ptr) {
					return TXScopeTaggedStrongConstPointerStore<_TStrongPointer, TagID>(std::forward<decltype(stored_ptr)>(stored_ptr));
				}

			}
		}
		template<size_t TagID, typename _TStrongPointer>
		auto make_xscope_tagged_strong_const_pointer_store(const _TStrongPointer& stored_ptr) {
			typedef typename std::remove_reference<_TStrongPointer>::type _TStrongPointerNR;
			return impl::ns_xscope_tagged_strong_pointer_store::make_xscope_tagged_strong_const_pointer_store_helper1<TagID, _TStrongPointerNR>(typename std::is_base_of<mse::us::impl::NeverNullTagBase, _TStrongPointerNR>::type(), stored_ptr);
		}
		template<size_t TagID, typename _TStrongPointer, class = MSE_IMPL_ENABLE_IF_NOT_RETURNABLE_FPARAM(_TStrongPointer)>
		auto make_xscope_tagged_strong_const_pointer_store(_TStrongPointer&& stored_ptr) {
			typedef typename std::remove_reference<_TStrongPointer>::type _TStrongPointerNR;
			return impl::ns_xscope_tagged_strong_pointer_store::make_xscope_tagged_strong_const_pointer_store_helper1<TagID, _TStrongPointerNR>(typename std::is_base_of<mse::us::impl::NeverNullTagBase, _TStrongPointerNR>::type(), std::forward<decltype(stored_ptr)>(stored_ptr));
		}

		namespace impl {
			namespace ns_xscope_tagged_strong_pointer_store {

				template<size_t TagID, typename _TStrongPointer>
				auto make_xscope_tagged_strong_pointer_store_helper1(std::true_type, const _TStrongPointer& stored_ptr) {
					return TXScopeTaggedStrongNotNullPointerStore<_TStrongPointer, TagID>(stored_ptr);
				}
				template<size_t TagID, typename _TStrongPointer>
				auto make_xscope_tagged_strong_pointer_store_helper1(std::true_type, _TStrongPointer&& stored_ptr) {
					return TXScopeTaggedStrongNotNullPointerStore<_TStrongPointer, TagID>(std::forward<decltype(stored_ptr)>(stored_ptr));
				}
				template<size_t TagID, typename _TStrongPointer>
				auto make_xscope_tagged_strong_pointer_store_helper1(std::false_type, const _TStrongPointer& stored_ptr) {
					return TXScopeTaggedStrongPointerStore<_TStrongPointer, TagID>(stored_ptr);
				}
				template<size_t TagID, typename _TStrongPointer>
				auto make_xscope_tagged_strong_pointer_store_helper1(std::false_type, _TStrongPointer&& stored_ptr) {
					return TXScopeTaggedStrongPointerStore<_TStrongPointer, TagID>(std::forward<decltype(stored_ptr)>(stored_ptr));
				}

				template<size_t TagID, typename _TStrongPointer>
				auto make_xscope_tagged_strong_pointer_store_helper2(std::true_type, const _TStrongPointer& stored_ptr) {
					return make_xscope_tagged_strong_const_pointer_store<TagID>(stored_ptr);
				}
				template<size_t TagID, typename _TStrongPointer>
				auto make_xscope_tagged_strong_pointer_store_helper2(std::true_type, _TStrongPointer&& stored_ptr) {
					return make_xscope_tagged_strong_const_pointer_store<TagID>(std::forward<decltype(stored_ptr)>(stored_ptr));
				}
				template<size_t TagID, typename _TStrongPointer>
				auto make_xscope_tagged_strong_pointer_store_helper2(std::false_type, const _TStrongPointer& stored_ptr) {
					typedef typename std::remove_reference<_TStrongPointer>::type _TStrongPointerNR;
					return make_xscope_tagged_strong_pointer_store_helper1<TagID>(typename std::is_base_of<mse::us::impl::NeverNullTagBase, _TStrongPointerNR>::type(), stored_ptr);
				}
				template<size_t TagID, typename _TStrongPointer>
				auto make_xscope_tagged_strong_pointer_store_helper2(std::false_type, _TStrongPointer&& stored_ptr) {
					typedef typename std::remove_reference<_TStrongPointer>::type _TStrongPointerNR;
					return make_xscope_tagged_strong_pointer_store_helper1<TagID>(typename std::is_base_of<mse::us::impl::NeverNullTagBase, _TStrongPointerNR>::type(), std::forward<decltype(stored_ptr)>(stored_ptr));
				}
			}
		}
		template<size_t TagID, typename _TStrongPointer>
		auto make_xscope_tagged_strong_pointer_store(const _TStrongPointer& stored_ptr) {
			typedef typename std::remove_reference<_TStrongPointer>::type _TStrongPointerNR;
			typedef typename std::remove_reference<decltype(*stored_ptr)>::type _TTargetNR;
			return impl::ns_xscope_tagged_strong_pointer_store::make_xscope_tagged_strong_pointer_store_helper2<TagID, _TStrongPointerNR>(typename std::is_const<_TTargetNR>::type(), stored_ptr);
		}
		template<size_t TagID, typename _TStrongPointer, class = MSE_IMPL_ENABLE_IF_NOT_RETURNABLE_FPARAM(_TStrongPointer)>
		auto make_xscope_tagged_strong_pointer_store(_TStrongPointer&& stored_ptr) {
			typedef typename std::remove_reference<_TStrongPointer>::type _TStrongPointerNR;
			typedef typename std::remove_reference<decltype(*stored_ptr)>::type _TTargetNR;
			return impl::ns_xscope_tagged_strong_pointer_store::make_xscope_tagged_strong_pointer_store_helper2<TagID, _TStrongPointerNR>(typename std::is_const<_TTargetNR>::type(), std::forward<decltype(stored_ptr)>(stored_ptr));
		}
		/* Overloads for rsv::TReturnableFParam<>. */
		MSE_OVERLOAD_FOR_RETURNABLE_FPARAM_DECLARATION(make_xscope_tagged_strong_const_pointer_store)
			MSE_OVERLOAD_FOR_RETURNABLE_FPARAM_DECLARATION(make_xscope_tagged_strong_pointer_store)

			template<typename _Ty, size_t TagID> using TXScopeTaggedXScopeTaggedItemFixedStore = TXScopeTaggedStrongNotNullPointerStore<TXScopeTaggedFixedPointer<_Ty, TagID>, TagID>;
		template<typename _Ty, size_t TagID> using TXScopeTaggedXScopeTaggedItemFixedConstStore = TXScopeTaggedStrongNotNullConstPointerStore<TXScopeTaggedFixedConstPointer<_Ty, TagID>, TagID>;
#if !defined(MSE_SCOPEPOINTER_DISABLED)
		template<typename _Ty, size_t TagID> using TXScopeTaggedXScopeTaggedFixedStore = TXScopeTaggedStrongNotNullPointerStore<TXScopeTaggedObjFixedPointer<_Ty, TagID>, TagID>;
		template<typename _Ty, size_t TagID> using TXScopeTaggedXScopeTaggedFixedConstStore = TXScopeTaggedStrongNotNullConstPointerStore<TXScopeTaggedObjFixedConstPointer<_Ty, TagID>, TagID>;
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)
	}


	namespace rsv {
		namespace impl {
			template<typename _Ty>
			class TContainsNonOwningScopeTaggedReferenceWrapper : public _Ty, public mse::us::impl::ContainsNonOwningScopeTaggedReferenceTagBase {
				typedef _Ty base_class;
			public:
				MSE_USING_AND_DEFAULT_COPY_AND_MOVE_CONSTRUCTOR_DECLARATIONS_AND_USING_ASSIGNMENT_OPERATOR(TContainsNonOwningScopeTaggedReferenceWrapper, _Ty);
				MSE_IMPL_DESTRUCTOR_PREFIX1 ~TContainsNonOwningScopeTaggedReferenceWrapper() {}
			};
		}
		template<typename _TLambda>
		auto make_xscope_tagged_reference_or_pointer_capture_lambda(const _TLambda& lambda) {
			return mse::rsv::make_xscope_tagged(mse::rsv::impl::TContainsNonOwningScopeTaggedReferenceWrapper<_TLambda>(lambda));
		}
		template<typename _TLambda>
		auto make_xscope_tagged_non_reference_or_pointer_capture_lambda(const _TLambda& lambda) {
			return mse::rsv::make_xscope_tagged(lambda);
		}
		template<typename _TLambda>
		auto make_xscope_tagged_capture_lambda(const _TLambda& lambda) {
			return make_xscope_tagged_non_reference_or_pointer_capture_lambda(lambda);
		}
		template<typename _TLambda>
		auto make_xscope_tagged_non_capture_lambda(const _TLambda& lambda) {
			return mse::rsv::make_xscope_tagged(lambda);
		}
	}

	namespace rsv {
		namespace us {
			namespace impl {
				/* The new() operator of scope objects is (often) private. The implementation of some elements (like those that
				use std::any<> or std::function<> type-erasure) may require access to the new() operator. This is just a
				transparent wrapper that doesn't "hide" its new() operator and can be used to wrap scope objects that do. */
				template<typename _TROy>
				class TNewableXScopeTaggedObj : public _TROy {
				public:
					typedef _TROy base_class;
					MSE_USING_AND_DEFAULT_COPY_AND_MOVE_CONSTRUCTOR_DECLARATIONS_AND_USING_ASSIGNMENT_OPERATOR(TNewableXScopeTaggedObj, base_class);

					MSE_DEFAULT_OPERATOR_NEW_DECLARATION
				};
				namespace impl {
					template<typename _Ty>
					auto make_newable_xscope_tagged_helper(std::false_type, const _Ty& arg) {
						/* Objects that don't derive from mse::us::impl::XScopeTaggedTagBase generally don't hide their new() operators
						and may not be usable as a base class. */
						return arg;
					}
					template<typename _Ty>
					auto make_newable_xscope_tagged_helper(std::false_type, _Ty&& arg) {
						/* Objects that don't derive from mse::us::impl::XScopeTaggedTagBase generally don't hide their new() operators
						and may not be usable as a base class. */
						return std::forward<decltype(arg)>(arg);
					}

					template<typename _Ty>
					auto make_newable_xscope_tagged_helper(std::true_type, const _Ty& arg) {
						return TNewableXScopeTaggedObj<_Ty>(arg);
					}
					template<typename _Ty>
					auto make_newable_xscope_tagged_helper(std::true_type, _Ty&& arg) {
						return TNewableXScopeTaggedObj<_Ty>(std::forward<decltype(arg)>(arg));
					}
				}
				template <class X>
				auto make_newable_xscope_tagged(const X& arg) {
					typedef typename std::remove_reference<X>::type nrX;
					return impl::make_newable_xscope_tagged_helper<nrX>(typename std::is_base_of<mse::us::impl::XScopeTaggedTagBase, nrX>::type(), arg);
				}
				template <class X>
				auto make_newable_xscope_tagged(X&& arg) {
					typedef typename std::remove_reference<X>::type nrX;
					return impl::make_newable_xscope_tagged_helper<nrX>(typename mse::impl::is_instantiation_of<nrX, TNewableXScopeTaggedObj>::type(), std::forward<decltype(arg)>(arg));
				}
			}
		}


		/* The purpose of the xscope_tagged_chosen_pointer() function is simply to take two scope pointers as input parameters and return (a copy
		of) one of them. Which of the pointers is returned is determined by a "decider" function that is passed, as the first parameter, to
		xscope_tagged_chosen_pointer(). The "decider" function needs to return a bool and take the two scope pointers as its first two parameters.
		The reason this xscope_tagged_chosen_pointer() function is needed is that (non-owning) scope pointers are, in general, not allowed to be
		used as a function return value. (Because you might accidentally return a pointer to a local scope object (which is bad)
		instead of one of the pointers given as an input parameter (which is fine).) So the xscope_tagged_chosen_pointer() template is the
		sanctioned way of creating a function that returns a non-owning scope pointer. */
		template<typename _TBoolFunction, typename _Ty, size_t TagID, class... Args>
		auto xscope_tagged_chosen_pointer(_TBoolFunction function1, const TXScopeTaggedFixedConstPointer<_Ty, TagID>& a, const TXScopeTaggedFixedConstPointer<_Ty, TagID>& b, Args&&... args) {
			return function1(a, b, std::forward<Args>(args)...) ? b : a;
		}
		template<typename _TBoolFunction, typename _Ty, size_t TagID, class... Args>
		auto xscope_tagged_chosen_pointer(_TBoolFunction function1, const TXScopeTaggedFixedPointer<_Ty, TagID>& a, const TXScopeTaggedFixedPointer<_Ty, TagID>& b, Args&&... args) {
			return function1(a, b, std::forward<Args>(args)...) ? b : a;
		}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
		template<typename _TBoolFunction, typename _Ty, size_t TagID, class... Args>
		auto xscope_tagged_chosen_pointer(_TBoolFunction function1, const TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& a, const TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& b, Args&&... args) {
			return function1(a, b, std::forward<Args>(args)...) ? b : a;
		}
		template<typename _TBoolFunction, typename _Ty, size_t TagID, class... Args>
		auto xscope_tagged_chosen_pointer(_TBoolFunction function1, const TXScopeTaggedObjFixedPointer<_Ty, TagID>& a, const TXScopeTaggedObjFixedPointer<_Ty, TagID>& b, Args&&... args) {
			return function1(a, b, std::forward<Args>(args)...) ? b : a;
		}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)

		template<typename _Ty, size_t TagID>
		auto xscope_tagged_chosen_pointer(bool choose_the_second, const TXScopeTaggedFixedConstPointer<_Ty, TagID>& a, const TXScopeTaggedFixedConstPointer<_Ty, TagID>& b) {
			return choose_the_second ? b : a;
		}
		template<typename _Ty, size_t TagID>
		auto xscope_tagged_chosen_pointer(bool choose_the_second, const TXScopeTaggedFixedPointer<_Ty, TagID>& a, const TXScopeTaggedFixedPointer<_Ty, TagID>& b) {
			return choose_the_second ? b : a;
		}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
		template<typename _Ty, size_t TagID>
		auto xscope_tagged_chosen_pointer(bool choose_the_second, const TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& a, const TXScopeTaggedObjFixedConstPointer<_Ty, TagID>& b) {
			return choose_the_second ? b : a;
		}
		template<typename _Ty, size_t TagID>
		auto xscope_tagged_chosen_pointer(bool choose_the_second, const TXScopeTaggedObjFixedPointer<_Ty, TagID>& a, const TXScopeTaggedObjFixedPointer<_Ty, TagID>& b) {
			return choose_the_second ? b : a;
		}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)
		template<typename _TBoolFunction, typename _Ty, class... Args>
		auto xscope_tagged_chosen(const _TBoolFunction& function1, const _Ty& a, const _Ty& b, Args&&... args) {
			return chosen(function1, a, b, std::forward<Args>(args)...);
		}
		template<typename _Ty>
		auto xscope_tagged_chosen(bool choose_the_second, const _Ty& a, const _Ty& b) {
			return chosen(choose_the_second, a, b);
		}

		/* shorter aliases */
		template<typename _TROy, size_t TagID> using sto = TXScopeTaggedObj<_TROy, TagID>;
		template<typename _Ty, size_t TagID> using stifp = TXScopeTaggedFixedPointer<_Ty, TagID>;
		template<typename _Ty, size_t TagID> using stifcp = TXScopeTaggedFixedConstPointer<_Ty, TagID>;

		template<typename _TROy, size_t TagID> using xst_obj = TXScopeTaggedObj<_TROy, TagID>;
		template<typename _Ty, size_t TagID> using xst_fptr = TXScopeTaggedFixedPointer<_Ty, TagID>;
		template<typename _Ty, size_t TagID> using xst_fcptr = TXScopeTaggedFixedConstPointer<_Ty, TagID>;
	}


	namespace self_test {
		namespace ns_XScpTaggedPtrTest1 {

#ifdef MSE_SELF_TESTS
			class A1 {
			public:
				A1(int x) : b(x) {}
				A1(const A1& _X) : b(_X.b) {}
				A1(A1&& _X) : b(std::forward<decltype(_X)>(_X).b) {}
				virtual ~A1() {}
				A1& operator=(A1&& _X) { b = std::forward<decltype(_X)>(_X).b; return (*this); }
				A1& operator=(const A1& _X) { b = _X.b; return (*this); }

				int b = 3;
			};
			class B1 {
			public:
				static int foo1(A1* a_native_ptr) { return a_native_ptr->b; }
				template<size_t TagID>
				static int foo2(mse::rsv::TXScopeTaggedFixedPointer<A1, TagID> A1_scope_tagged_ptr) { return A1_scope_tagged_ptr->b; }
			protected:
				~B1() {}
			};

			class A2 {
			public:
				A2(int x) : b(x) {}
				virtual ~A2() {}

				int b = 3;
				std::string s = "some text ";
			};
			class B2 {
			public:
				static int foo1(A2* a_native_ptr) { return a_native_ptr->b; }
				template<size_t TagID>
				static int foo2(mse::rsv::TXScopeTaggedFixedPointer<A2, TagID> A2_scpfptr) { return A2_scpfptr->b; }
				template<class TPtr>
				static int foo3(TPtr A2_ptr) { return A2_ptr->b; }
				static int foo4(mse::TXScopeFixedPointer<A2> A2_scpfcptr) { return A2_scpfcptr->b; }
			protected:
				~B2() {}
			};
#endif // MSE_SELF_TESTS


			class CXScpTaggedPtrTest1 {
			public:
				static void s_test1() {
#ifdef MSE_SELF_TESTS

					A1* A1_native_ptr = nullptr;

					{
						A1 a(7);
						mse::rsv::TXScopeTaggedObj<A1, MSE_NEWTAG> scope_tagged_a(7);
						/* mse::rsv::TXScopeTaggedObj<A1> is a class that is publicly derived from A1, and so should be a compatible substitute for A1
						in almost all cases. */

						assert(a.b == scope_tagged_a.b);
						A1_native_ptr = &a;

						mse::rsv::TXScopeTaggedFixedPointer<A1, MSE_NEWTAG> A1_scope_tagged_ptr1(&scope_tagged_a);
						assert(A1_native_ptr->b == A1_scope_tagged_ptr1->b);
						mse::rsv::TXScopeTaggedFixedPointer<A1, MSE_NEWTAG> A1_scope_tagged_ptr2 = &scope_tagged_a;

						if (!A1_scope_tagged_ptr2) {
							assert(false);
						}
						else if (!(A1_scope_tagged_ptr2 != A1_scope_tagged_ptr1)) {
							int q = B1::foo2(A1_scope_tagged_ptr2);
						}
						else {
							assert(false);
						}

						mse::us::impl::TPointerForLegacy<A1> pfl_ptr1 = &a;
						if (!(pfl_ptr1 != nullptr)) {
							assert(false);
						}
						mse::us::impl::TPointerForLegacy<A1> pfl_ptr2 = nullptr;
						if (!(pfl_ptr1 != pfl_ptr2)) {
							assert(false);
						}

						A1 a2 = a;
						mse::rsv::TXScopeTaggedObj<A1, MSE_NEWTAG> scope_tagged_a2 = scope_tagged_a;
						scope_tagged_a2 = a;
						scope_tagged_a2 = scope_tagged_a;

						mse::rsv::TXScopeTaggedFixedConstPointer<A1, MSE_NEWTAG> rcp = A1_scope_tagged_ptr1;
						mse::rsv::TXScopeTaggedFixedConstPointer<A1, MSE_NEWTAG> rcp2 = rcp;
						const mse::rsv::TXScopeTaggedObj<A1, MSE_NEWTAG> cscope_tagged_a(11);
						mse::rsv::TXScopeTaggedFixedConstPointer<A1, MSE_NEWTAG> rfcp = &cscope_tagged_a;

						mse::rsv::TXScopeTaggedOwnerPointer<A1, MSE_NEWTAG> A1_scpoptr(11);
						//B1::foo2(A1_scpoptr);
						B1::foo2(&*A1_scpoptr);
						if (A1_scpoptr->b == (&*A1_scpoptr)->b) {
						}
					}

					{
						/* Polymorphic conversions. */
						class E {
						public:
							int m_b = 5;
						};

						/* Polymorphic conversions that would not be supported by mse::TRegisteredPointer. */
						class GE : public E {};
						mse::rsv::TXScopeTaggedObj<GE, MSE_NEWTAG> scope_tagged_ge;
						mse::rsv::TXScopeTaggedFixedPointer<GE, MSE_NEWTAG> GE_scope_tagged_ifptr1 = &scope_tagged_ge;
						mse::rsv::TXScopeTaggedFixedPointer<E, MSE_NEWTAG> E_scope_tagged_ifptr5 = GE_scope_tagged_ifptr1;
						mse::rsv::TXScopeTaggedFixedPointer<E, MSE_NEWTAG> E_scope_tagged_ifptr2(&scope_tagged_ge);
						mse::rsv::TXScopeTaggedFixedConstPointer<E, MSE_NEWTAG> E_scope_tagged_fcptr2 = &scope_tagged_ge;
					}

					{
						mse::rsv::TXScopeTaggedObj<A2, MSE_NEWTAG> a_scpobj(5);
						int res1 = (&a_scpobj)->b;
						int res2 = B2::foo2(&a_scpobj);
						int res3 = B2::foo3(&a_scpobj);
						mse::rsv::TXScopeTaggedOwnerPointer<A2, MSE_NEWTAG> a_scpoptr(7);
						//int res4 = B2::foo2(a_scpoptr);
						int res4b = B2::foo2(&(*a_scpoptr));

						/* You can use the "mse::rsv::make_xscope_tagged_pointer_to_member_v2()" function to obtain a safe pointer to a member of
						an xscope object. */
						auto s_safe_ptr1 = mse::rsv::make_xscope_tagged_pointer_to_member_v2((&a_scpobj), &A2::s);
						(*s_safe_ptr1) = "some new text";
						auto s_safe_const_ptr1 = mse::rsv::make_xscope_tagged_const_pointer_to_member_v2((&a_scpobj), &A2::s);
					}

					{
						int a(7);
						mse::rsv::TXScopeTaggedObj<int, MSE_NEWTAG> scope_tagged_a(7);
						/* Use of scalar types (that can't be used as base class types) with TXScopeTaggedObj<> is not well supported. So,
						for example, rather than mse::rsv::TXScopeTaggedObj<int>, mse::rsv::TXScopeTaggedObj<mse::CInt> would be preferred. */

						auto int_native_ptr = &a;

						mse::rsv::TXScopeTaggedFixedPointer<int, MSE_NEWTAG> int_scope_tagged_ptr1 = &scope_tagged_a;
						mse::rsv::TXScopeTaggedFixedPointer<int, MSE_NEWTAG> int_scope_tagged_ptr2 = int_scope_tagged_ptr1;
						//mse::TXScopeFixedPointer<int> int_scope_ptr3 = &scope_tagged_a;

						if (!int_scope_tagged_ptr2) {
							assert(false);
						}
						else if (!(int_scope_tagged_ptr2 != int_scope_tagged_ptr1)) {
							int q = 5;
						}
						else {
							assert(false);
						}

						int a2 = a;
						mse::rsv::TXScopeTaggedObj<int, MSE_NEWTAG> scope_tagged_a2 = scope_tagged_a;
						scope_tagged_a2 = a;
						scope_tagged_a2 = scope_tagged_a;

						mse::rsv::TXScopeTaggedFixedConstPointer<int, MSE_NEWTAG> rcp = int_scope_tagged_ptr1;
						mse::rsv::TXScopeTaggedFixedConstPointer<int, MSE_NEWTAG> rcp2 = rcp;
						const mse::rsv::TXScopeTaggedObj<int, MSE_NEWTAG> cscope_tagged_a(11);
						mse::rsv::TXScopeTaggedFixedConstPointer<int, MSE_NEWTAG> rfcp = &cscope_tagged_a;

						mse::rsv::TXScopeTaggedOwnerPointer<int, MSE_NEWTAG> int_scpoptr(11);
						auto int_scpptr = &*int_scpoptr;
					}

#endif // MSE_SELF_TESTS
				}
			};
		}
	}
}

#ifndef MSE_PUSH_MACRO_NOT_SUPPORTED
#pragma pop_macro("_NOEXCEPT")
#endif // !MSE_PUSH_MACRO_NOT_SUPPORTED

#ifdef __clang__
#pragma clang diagnostic pop
#else /*__clang__*/
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif /*__GNUC__*/
#endif /*__clang__*/

#ifdef _MSC_VER
#pragma warning( pop )  
#endif /*_MSC_VER*/

#endif // MSESCOPE_TAGGED_H_
