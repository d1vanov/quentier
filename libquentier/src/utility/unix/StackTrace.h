/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (c) 2009, Ethan Tira-Thompson, Fredrik Orderud.
 *
 * License: New BSD License (BSD) and LGPL.
 *          http://www.opensource.org/licenses/bsd-license.php
 *          http://www.opensource.org/licenses/lgpl-2.1.php
 *
 * Webpage: http://stacktrace.sourceforge.net/
 */
#ifndef LIB_QUENTIER_UTILITY_UNIX_STACK_TRACE_H
#define LIB_QUENTIER_UTILITY_UNIX_STACK_TRACE_H

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif
// Aperios and Mac OS X 10.4 and prior need the "manual" unroll; otherwise assume execinfo.h and backtrace() are available
#if !defined(PLATFORM_APERIOS) && (!defined(__APPLE__) || defined(MAC_OS_X_VERSION_10_5))
#define STACKTRACE_USE_BACKTRACE
#endif

#ifndef __cplusplus
#include <stddef.h>
#else // __cplusplus
#include <cstddef>
//! Holds the C-style interface for the stack trace routines
namespace stacktrace {
extern "C" {
#endif // __cplusplus

    typedef int machineInstruction; //!< typedef in case type needs to change on other platforms (i.e. long for 64 bit architectures?)

    //! Stores information about a single stack frame
    struct StackFrame {

#ifdef STACKTRACE_USE_BACKTRACE
        //! pointer to array of return addresses, shared by all StackFrames in the list
        void *** packedRA;
        //! pointer to number of entries available at #packedRA
        size_t * packedRAUsed;
        //! pointer to size of array at #packedRA
        size_t * packedRACap;
        //! entry in #packedRA corresponding to this frame
        size_t depth;
        //! return address, points to instruction being executed within current frame
        void * ra;
#else
        //! stack pointer, points to end of stack frame
        void * sp;

#  if defined(__PIC__) && (defined(__MIPSEL__) || defined(__MIPS__))
        //! return address, points to instruction being executed within current frame
        /*! Note that this is the address that is being returned to by the @e sub-function,
         *  @e not the address that this frame itself is returning to.
         *  When executing position independent code (PIC), this is the address relative
         *  to #gp.  In other words, subtract this from #gp to get current memory address,
         *  or subtract from the binary's _gp symbol to get link-time address (for looking up file/line) */
        size_t ra;

        //! global offset used in position independent code (PIC)
        /*! subtract #ra from this to get the actual run-time memory address of the instruction */
        size_t gp;

#  else
        //! return address, points to instruction being executed within current frame
        /*! Note that this is the address that is being returned to by the @e sub-function,
         *  @e not the address that this frame itself is returning to. */
        machineInstruction * ra;
#  endif /* __PIC__ */

#  ifdef DEBUG_STACKTRACE
        //! if DEBUG_STACKTRACE is defined, this field is available which, if non-zero, will cause debugging info to be displayed on each unroll
        int debug;
#  endif
#endif

        //! points to the caller's stack frame (stack frame of function which called this one), may be NULL or self-referential at end of list
        /*! a self-referential value indicates the frame is not the last on the stack, but is the last recorded. */
        struct StackFrame * caller;

    };

    //! stores information about the caller's stack frame into @a frame
    void getCurrentStackFrame(struct StackFrame * frame);

    //! stores information about the caller to @a curFrame into @a nextFrame
    /*! @return 0 if error occurred (i.e. bottom of the stack), non-zero upon success
     *  @a nextFrame @e can be the same instance as @a curFrame, will update in place.
     *  @a curFrame->caller will be set to @a nextFrame. */
    int unrollStackFrame(struct StackFrame * curFrame, struct StackFrame * nextFrame);

    //! frees a list of StackFrames, such as is returned by recordStackTrace
    void freeStackTrace(struct StackFrame * frame);

    //! preallocates a stack trace of a particular size (doesn't actually perform a stack trace, merely allocates the linked list)
    /*! this is a good idea if you want to do a stack trace within an exception handler, which might have been triggered by running out of heap */
    struct StackFrame * allocateStackTrace(unsigned int size);

    //! dumps stored stack trace to stderr
    void displayStackTrace(const struct StackFrame * frame);

#ifndef __cplusplus

    //! dumps current stack trace to stderr, up to @a limit depth and skipping the top @a skip frames
    /*! pass -1U for limit to request unlimited trace, and 0 to start with the function calling recordStackTrace */
    void displayCurrentStackTrace(unsigned int limit, unsigned int skip);

    //! repeatedly calls unrollStackFrame() until the root frame is reached or @a limit is hit, skipping the top @a skip frames
    /*! pass -1U for limit to request unlimited trace, and 0 to start with the function calling recordStackTrace */
    struct StackFrame * recordStackTrace(unsigned int limit, unsigned int skip);
    //! repeatedly calls unrollStackFrame() until the root frame is reached or end of @a frame list is hit, skipping the top @a skip frames
    /*! This is handy for reusing previously allocated frames, returns the unused portion (if return value equals @a frame, none were used -- implies never cleared @a skip) */
    struct StackFrame * recordOverStackTrace(struct StackFrame * frame, unsigned int skip);

#else // __cplusplus

    //! dumps current stack trace to stderr, up to @a limit depth and skipping the top @a skip frames
    /*! pass -1U for limit to request unlimited trace, and 0 to start with the function calling recordStackTrace */
    void displayCurrentStackTrace(unsigned int limit = -1U, unsigned int skip = 0);

    //! repeatedly calls unrollStackFrame() until the root frame is reached or @a limit is hit, skipping the top @a skip frames
    /*! pass -1U for limit to request unlimited trace, and 0 to start with the function calling recordStackTrace */
    struct StackFrame * recordStackTrace(unsigned int limit = -1U, unsigned int skip = 0);
    //! repeatedly calls unrollStackFrame() until the root frame is reached or end of @a frame list is hit, skipping the top @a skip frames
    /*! This is handy for reusing previously allocated frames, returns the unused portion (if return value equals @a frame, none were used -- implies never cleared @a skip) */
    struct StackFrame * recordOverStackTrace(struct StackFrame * frame, unsigned int skip = 0);

} // extern "C"
} // namespace stacktrace

#endif // __cplusplus

/*! @file
 * @brief Describes functionality for performing stack traces
 * @author ejt (Creator)
 *
 * $Author: ejtttje $
 * $Name:  $
 * $Revision: 1.2 $
 * $State: Exp $
 * $Date: 2009/11/20 00:50:23 $
 */
#endif
