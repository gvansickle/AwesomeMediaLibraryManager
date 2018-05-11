/*
 * crtp.h
 *
 *  Created on: May 10, 2018
 *      Author: gary
 */

#ifndef SRC_UTILS_CRTP_H_
#define SRC_UTILS_CRTP_H_

/**
 * CRTP helper
 * - Allows CRTP use in class hierarchies while avoiding the diamond problem
 *    (I was never able to get virt. inheritance to help with that here)
 * - Allows public derivation from multiple CRTP base classes. [1]
 * - Eliminates the need for the derived CRTP class to static_cast<> itself to the derived class; use the
 *   "underlying()" members instead.
 * Per https://www.fluentcpp.com/2017/05/19/crtp-helper/
 *
 * @note [1] Still need to do this in client classes to avoid ambiguous name resolution:
 *     using MyCRTPBase<MyClass>::funcInMyCRTPBase;
 * @see https://stackoverflow.com/a/46916924
 * This could maybe be macrozied to make it a bit cleaner.
 */
template <typename T, template<typename> class crtpType>
struct crtp
{
	/**
	 * Use these in derived classes like so:
	 * @code
	 *    this->underlying().SomeCRTPedFunction();
	 * @endcode
	 */
    T& underlying() { return static_cast<T&>(*this); }
    T const& underlying() const { return static_cast<T const&>(*this); }
private:
    crtp(){}
    friend crtpType<T>;
};


#endif /* SRC_UTILS_CRTP_H_ */
