#ifndef __PAIRUTILS_H__
#define __PAIRUTILS_H__

namespace stdex {
	namespace impl_ {
		template <typename T>
		class set_first_holder {
			T val;
		public:
			set_first_holder(const T& val_) : val(val_) { }
	
			template <typename X>
			void operator()(std::pair<T, X>& p) const {
				p.first = val;
			}
		};

		template <typename T>
		class set_second_holder {
			T val;
		public:
			set_second_holder(const T& val_) : val(val_) { }

			template <typename X>
			void operator()(std::pair<X, T>& p) const {
				p.second = val;
			}
		};

		template <typename T1, typename T2>
		class set_both_holder {
			T1 val_first;
			T2 val_second;
		public:
			set_both_holder(const T1& val_first_, const T2& val_second_) : val_first(val_first_), val_second(val_second_) { }

			void operator()(std::pair<T1, T2>& p) const {
				p.first = val_first;
				p.second = val_second;
			}
		};
	}

	template <typename T>
	impl_::set_first_holder<T> set_first(const T& val) { return impl_::set_first_holder<T>(val); }


	template <typename T>
	impl_::set_second_holder<T> set_second(const T& val) { return impl_::set_second_holder<T>(val); }


	template <typename T1, typename T2>
	impl_::set_both_holder<T1, T2> set_second(const T1& val_first, const T2& val_second) { 
		return impl_::set_both_holder<T1, T2>(val_first, val_second); 
	}
}

#endif // __PAIRUTILS_H__