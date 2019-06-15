//
// Created by gary on 6/14/19.
//

/**
 * @file UndoRedoHelper.h
 * Helper macros for Undo/Redo.
 * Borrowed from KDenLive's undohelper.hpp and macros.hpp.
 */

#ifndef AWESOMEMEDIALIBRARYMANAGER_UNDOREDOHELPER_H
#define AWESOMEMEDIALIBRARYMANAGER_UNDOREDOHELPER_H

#include <functional>



using Fun = std::function<bool(void)>;

/**
 * This macro executes an operation after a given lambda.
 */
#define PUSH_LAMBDA(operation, lambda) \
	lambda = [lambda, operation]() { \
		bool v = lambda(); \
		return v && operation(); \
	};

/**
 * This macro executes an operation before a given lambda
 */
#define PUSH_FRONT_LAMBDA(operation, lambda) \
	lambda = [lambda, operation]() { \
		bool v = operation(); \
		return v && lambda(); \
	};

/*  This file contains a collection of macros that can be used in model related classes.
	The class only needs to have the following members:
	- For Push_undo : std::weak_ptr<DocUndoStack> m_undoStack;  this is a pointer to the undoStack
	- For Update_undo_redo: mutable QReadWriteLock m_lock; This is a lock that ensures safety in case of concurrent access. Note that the mutex must be
   recursive.

	See for example TimelineModel.

	Note that there also exists a version of update_undo_redo without the need for a lock (but prefer the mutex version where applicable)
*/

/* This convenience macro adds lock/unlock ability to a given lambda function
   Note that it is automatically called when you push the lambda so you shouldn't have
   to call it directly yourself
*/
#define LOCK_IN_LAMBDA(mutex, lambda) \
	lambda = [this, lambda]() { \
		std::unique_lock write_lock(mutex); \
		bool res_lambda = lambda(); \
		return res_lambda; \
	};

/*This convenience macro locks the mutex for reading.
Note that it might happen that a thread is executing a write operation that requires
reading a Read-protected property. In that case, we try to write lock it first (this will be granted since the lock is recursive)
*/
//#define READ_LOCK()                                                                                                                                            \
//	std::unique_ptr<QReadLocker> rlocker(new QReadLocker(nullptr));                                                                                            \
//	std::unique_ptr<QWriteLocker> wlocker(new QWriteLocker(nullptr));                                                                                          \
//	if (m_lock.tryLockForWrite()) {                                                                                                                            \
//		/*we yield ownership of the lock to the WriteLocker*/                                                                                                  \
//		m_lock.unlock();                                                                                                                                       \
//		wlocker.reset(new QWriteLocker(&m_lock));                                                                                                              \
//	} else {                                                                                                                                                   \
//		rlocker.reset(new QReadLocker(&m_lock));                                                                                                               \
//	}

/* @brief This macro takes some lambdas that represent undo/redo for an operation and the text (name) associated with this operation
   The lambdas are transformed to make sure they lock access to the class they operate on.
   Then they are added on the undoStack
*/
#define PUSH_UNDO(undo, redo, text)                                                                                                                            \
	if (auto ptr = m_undoStack.lock()) {                                                                                                                       \
		ptr->push(new FunctionalUndoCommand(undo, redo, text));                                                                                                \
	} else {                                                                                                                                                   \
		qDebug() << "ERROR : unable to access undo stack";                                                                                                     \
		Q_ASSERT(false);                                                                                                                                       \
	}

/* @brief This macro takes as parameter one atomic operation and its reverse, and update
   the undo and redo functional stacks/queue accordingly
   This should be used in the rare case where we don't need a lock mutex. In general, prefer the other version
*/
#define UPDATE_UNDO_REDO_NOLOCK(operation, reverse, undo, redo)                                                                                                \
	undo = [reverse, undo]() {                                                                                                                                 \
		bool v = reverse();                                                                                                                                    \
		return undo() && v;                                                                                                                                    \
	};                                                                                                                                                         \
	redo = [operation, redo]() {                                                                                                                               \
		bool v = redo();                                                                                                                                       \
		return operation() && v;                                                                                                                               \
	};
/* @brief This macro takes as parameter one atomic operation and its reverse, and update
   the undo and redo functional stacks/queue accordingly
   It will also ensure that operation and reverse are dealing with mutexes
*/
#define UPDATE_UNDO_REDO(mutex, operation, reverse, undo, redo)                                                                                                       \
	LOCK_IN_LAMBDA(mutex, operation)                                                                                                                                  \
	LOCK_IN_LAMBDA(mutex, reverse)                                                                                                                                    \
	UPDATE_UNDO_REDO_NOLOCK(operation, reverse, undo, redo)

inline static Fun noop_undo_redo_lambda = [](){ return true; };

#endif //AWESOMEMEDIALIBRARYMANAGER_UNDOREDOHELPER_H
