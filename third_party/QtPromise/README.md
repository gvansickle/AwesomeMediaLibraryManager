# QtPromise #

> Promise Pattern for Qt

[![Linux build status](https://gitlab.com/julrich/QtPromise/badges/master/build.svg)](https://gitlab.com/julrich/QtPromise/pipelines?scope=branches)
[![Windows build status](https://ci.appveyor.com/api/projects/status/s5bsjsbnj86sm42c/branch/master?svg=true)](https://ci.appveyor.com/project/j-ulrich/qtpromise/branch/master)
[![Coverage report](https://gitlab.com/julrich/QtPromise/badges/master/coverage.svg)](https://julrich.gitlab.io/QtPromise/coverage/)

### Table of Contents ###
> - [Motivation](#motivation)
>   - [Other Implementations](#other-implementations)
>   - [Further Reading](#further-reading)
> - [Features](#features)
> - [Examples](#examples)
> - [Requirements](#requirements)
> - [Documentation](#documentation)
> - [License](#license)


<a name="motivation"></a>
## Motivation ##
Working with asynchronous data (e.g. data fetched from web servers) can be cumbersome in Qt applications. Especially if the data needs to be processed before being usable by the application (think extracting specific information out of a JSON object) or if multiple data sources are involved.
Typically, it leads to splitting the processing across multiple signals and slots and having member variables only to remember the state of the processing between the processing steps.

The [promise pattern](https://en.wikipedia.org/wiki/Futures_and_promises) solves these problems by providing an object which represents the state of an asynchronous operation and allowing to chain processing steps to that object.
For more information about the promise pattern, see the links in the [Further Reading](#further-reading) section.

> **Note:** There are different namings for the pattern and it's concepts.
> Some use "future" and "promise" (especially the C++ implementations), others use "deferred" and "promise" (especially the JavaScript implementations), others use just "promise".
> The chaining is also called "pipelining" or "continuation".

<a name="other-implementations"></a>
### Other Implementations ###
[Boost::Future](http://www.boost.org/doc/libs/1_63_0/doc/html/thread/synchronization.html#thread.synchronization.futures.then) provides an implementation of the pattern.
The C++ Standard Library contains an experimental specification of the pattern: [std::experimental::future::then](http://en.cppreference.com/w/cpp/experimental/future/then).
Both do not integrate with Qt's signal & slot mechanism out of the box. On the other hand, they support exceptions.

Qt's [QFuture](http://doc.qt.io/qt-5.6/qfuture.html) and [QFutureWatcher](http://doc.qt.io/qt-5.6/qfuturewatcher.html) provide a similar functionality.
However, they do not allow chaining, enforce using threads and do not support custom asynchronous operations well (e.g. no progress reporting).

Ben Lau's [AsyncFuture](https://github.com/benlau/asyncfuture) is based on QFuture and adds some of the missing functionality.
However, it is using a different API, more oriented to the Observable pattern, and does not support progress reporting (yet?).

Benoit Walter's [QtPromise](https://github.com/bwalter/qt-promise) is based on Ben Lau's AsyncFuture
and provides an API similar to this implementation of QtPromise. However, it has some difference:
It uses context objects to manage lifetime of promises and to store additional data. It also doesn't provide progress reporting (yet?).

Simon Brunel's [QtPromise](https://github.com/simonbrunel/qtpromise) is an implementation which is very close to the Promises/A+ specification.
In contrast to this implementation of QtPromise, it uses static typing instead of dynamic typing (templates instead of QVariant) and has
nice support for exceptions. However, it does not provide progress reporting (yet?).

<a name="further-reading"></a>
### Further reading ###

- [Wikipedia: Futures and Promises](https://en.wikipedia.org/wiki/Futures_and_promises)
- [Promises/A+](https://promisesaplus.com)
- [JavaScript Promise (MDN)](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise)
- [AngularJS: $q](https://docs.angularjs.org/api/ng/service/$q)
- [jQuery: Deferred Object](https://api.jquery.com/category/deferred-object)
- [Boost::Future](http://www.boost.org/doc/libs/1_63_0/doc/html/thread/synchronization.html#thread.synchronization.futures.then)
- [std::experimental::future::then](http://en.cppreference.com/w/cpp/experimental/future/then)


<a name="features"></a>
## Features ##
- Promise chaining
- Error handling and progress reporting
> **Note:** QtPromise does **not** support throwing exceptions from Promise chain callbacks since Qt's signal & slot mechanism does not support exceptions.
However, exceptions can be [wrapped and handled](https://julrich.gitlab.io/QtPromise/docs/page_q_variant.html#using-smart-pointers) using the error handling.
- Can be used with or without threads
- Can be used with any asynchronous operation
- Supports signals & slots
- Supports `QNetworkReply`
- Supports `QFuture`
- [Semantic versioning](http://semver.org/)
> **Important note:** The library currently does **not** provide binary compatibility between versions. Only source compatibility is guaranteed between minor and patch versions.


<a name="examples"></a>
## Examples ##

### Custom asynchronous Operation ###
Provides the result of an asynchronous operation as a Promise.
```cpp
QtPromise::Promise::Ptr MyClass::startAsyncOperation()
{
	using namespace QtPromise;
	
	Deferred::Ptr deferred = Deferred::create();
	MyAsyncOperation* asyncOp = new MyAsyncOperation(this);
	
	QObject::connect(asyncOp, &MyAsyncOperation::success, [=](const QByteArray& data) {
		deferred->resolve(QVariant::fromValue(data));
	});
	QObject::connect(asyncOp, &MyAsyncOperation::failed, [=](const QString& error) {
		deferred->reject(QVariant::fromValue(error));
	});
	QObject::connect(asyncOp, &MyAsyncOperation::progress, [=](int current, int total) {
		QMap<int> progressMap;
		progressMap["current"] = current;
		progressMap["total"] = total;
		deferred->notify(QVariant::fromValue(progressMap));
	});
	
	Promise::Ptr promise = Promise::create(deferred)
	->always([=](const QVariant&) {
		// Delete asyncOp once the operation finished
		asyncOp->deleteLater();
	});
	
	asyncOp->start();
	return promise;
}
```

### Fetch JSON Data ###
Implements and uses a helper method to process JSON data fetched using a QNetworkReply.
```cpp
QtPromise::Promise::Ptr fetchJson(QNetworkReply* reply)
{
	return QtPromise::NetworkPromise::create(reply)
	->then([](const QVariant& data) -> QVariant {
		/* We could do more pre-processing here like
		 * removing comments from the JSON etc.
		 */
		return QJsonDocument::fromJson(data.value<QtPromise::NetworkDeferred::ReplyData>().data);
	});
}

// ...

void MyDataFetcher::printJsonData()
{
	QNetworkRequest request("http://api.example.com/getJsonData");
	QNetworkReply* reply = this->qnam->get(request);

	this->m_promise = fetchJson(reply)
	->then([this](const QVariant& data) {
		// Do something with JSON document
		this->print(data.toJsonDocument().toObject());
	}, [this](const QVariant& error) {
		this->logError("Error fetching JSON document: "+error.value<QtPromise::NetworkDeferred::Error>().message);
	});
}
```


<a name="requirements"></a>
## Requirements ##
 - Qt 5.4 or later
 - Compiler supporting C++11 (tested with Microsoft Visual Studio 2015, GCC 4.9.2 and GCC 6.3.0)
 - An event loop which also handles [DeferredDelete](http://doc.qt.io/qt-5/qevent.html#Type-enum) events
 (e.g. `QCoreApplication::exec()` or `QEventLoop::exec()`)


<a name="documentation"></a>
## Documentation ##
- [API Documentation](https://julrich.gitlab.io/QtPromise/docs/)
- [Guides](https://julrich.gitlab.io/QtPromise/docs/pages.html)
- [Changelog](CHANGELOG.md)


<a name="license"></a>
## License ##
Copyright (c) 2017 Jochen Ulrich

QtPromise is licensed under [MIT license](LICENSE).