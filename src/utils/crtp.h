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
 * - Eliminates the need for the derived CRTP class to static_cast<> itself to the derived class; in
 *   classes derived from crtp<>, use the "underlying()" members instead.
 *   For MixedIntoClass->crtp<> however, see note 1 below.
 * Per https://www.fluentcpp.com/2017/05/19/crtp-helper/
 *
 * Use like this:
 *
 * @code
 * // New class 1 that you want to use via CRTP.
 * template <class T>
 * class CRTP1 : crtp<T, CRTP1> ...
 *
 * // New class 2 that you want to use via CRTP.
 * template <class T>
 * class CRTP2 : crtp<T, CRTP2> ...
 *
 * // Class you want to use the above two CRTP classes with.
 * class NewClass : public CRTP1<NewClass>, public CRTP2<NewClass> ...
 *
 * @endcode
 *
 * So the top-level class gets "transported" all the way down the MI hierarchy in the first template parameter,
 * and the second parameter is used only to break the diamond inheritance.
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

	/**
     * Use these in derived classes like so:
     * @code
     *    this->underlying().SomeCRTPedFunction();
	 * @endcode
	 */
    T const& underlying() const { return static_cast<T const&>(*this); }

private:
    constexpr crtp() = default;

	/// crtpType<> exists only to differentiate types.
    friend crtpType<T>;
};


#endif /* SRC_UTILS_CRTP_H_ */
