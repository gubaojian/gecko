/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsWrapperCache_h___
#define nsWrapperCache_h___

#include "nsCycleCollectionParticipant.h"
#include "mozilla/Assertions.h"
#include "js/Id.h"          // must come before js/RootingAPI.h
#include "js/Value.h"       // must come before js/RootingAPI.h
#include "js/RootingAPI.h"
#include "js/TracingAPI.h"

class XPCWrappedNativeScope;

#define NS_WRAPPERCACHE_IID \
{ 0x6f3179a1, 0x36f7, 0x4a5c, \
  { 0x8c, 0xf1, 0xad, 0xc8, 0x7c, 0xde, 0x3e, 0x87 } }

/**
 * Class to store the wrapper for an object. This can only be used with objects
 * that only have one non-security wrapper at a time (for an XPCWrappedNative
 * this is usually ensured by setting an explicit parent in the PreCreate hook
 * for the class).
 *
 * An instance of nsWrapperCache can be gotten from an object that implements
 * a wrapper cache by calling QueryInterface on it. Note that this breaks XPCOM
 * rules a bit (this object doesn't derive from nsISupports).
 *
 * The cache can store objects other than wrappers. We allow wrappers to use a
 * separate JSObject to store their state (mostly expandos). If the wrapper is
 * collected and we want to preserve this state we actually store the state
 * object in the cache.
 *
 * The cache can store 2 types of objects:
 *
 *  If WRAPPER_IS_DOM_BINDING is not set (IsDOMBinding() returns false):
 *    - a slim wrapper or the JSObject of an XPCWrappedNative wrapper
 *
 *  If WRAPPER_IS_DOM_BINDING is set (IsDOMBinding() returns true):
 *    - a DOM binding object (regular JS object or proxy)
 *
 * The finalizer for the wrapper clears the cache.
 *
 * A number of the methods are implemented in nsWrapperCacheInlines.h because we
 * have to include some JS headers that don't play nicely with the rest of the
 * codebase. Include nsWrapperCacheInlines.h if you need to call those methods.
 */
class nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_WRAPPERCACHE_IID)

  nsWrapperCache() : mWrapper(nullptr), mFlags(0)
  {
  }
  ~nsWrapperCache()
  {
    MOZ_ASSERT(!PreservingWrapper(),
               "Destroying cache with a preserved wrapper!");
  }

  /**
   * Get the cached wrapper.
   *
   * This getter clears the gray bit before handing out the JSObject which means
   * that the object is guaranteed to be kept alive past the next CC.
   */
  JSObject* GetWrapper() const;

  /**
   * Get the cached wrapper.
   *
   * This getter does not change the color of the JSObject meaning that the
   * object returned is not guaranteed to be kept alive past the next CC.
   *
   * This should only be called if you are certain that the return value won't
   * be passed into a JS API function and that it won't be stored without being
   * rooted (or otherwise signaling the stored value to the CC).
   */
  JSObject* GetWrapperPreserveColor() const
  {
    return GetWrapperJSObject();
  }

  void SetWrapper(JSObject* aWrapper)
  {
    MOZ_ASSERT(!PreservingWrapper(), "Clearing a preserved wrapper!");
    MOZ_ASSERT(aWrapper, "Use ClearWrapper!");

    SetWrapperJSObject(aWrapper);
  }

  /**
   * Clear the wrapper. This should be called from the finalizer for the
   * wrapper.
   */
  void ClearWrapper()
  {
    MOZ_ASSERT(!PreservingWrapper(), "Clearing a preserved wrapper!");

    SetWrapperJSObject(nullptr);
  }

  bool PreservingWrapper()
  {
    return HasWrapperFlag(WRAPPER_BIT_PRESERVED);
  }

  void SetIsDOMBinding()
  {
    MOZ_ASSERT(!mWrapper && !(GetWrapperFlags() & ~WRAPPER_IS_DOM_BINDING),
               "This flag should be set before creating any wrappers.");
    SetWrapperFlags(WRAPPER_IS_DOM_BINDING);
  }

  bool IsDOMBinding() const
  {
    return HasWrapperFlag(WRAPPER_IS_DOM_BINDING);
  }

  /**
   * Wrap the object corresponding to this wrapper cache. If non-null is
   * returned, the object has already been stored in the wrapper cache.
   */
  virtual JSObject* WrapObject(JSContext* cx)
  {
    MOZ_ASSERT(!IsDOMBinding(), "Someone forgot to override WrapObject");
    return nullptr;
  }

  /**
   * Returns true if the object has a non-gray wrapper.
   */
  bool IsBlack();

  /**
   * Returns true if the object has a black wrapper,
   * and all the GC things it is keeping alive are black too.
   */
  bool IsBlackAndDoesNotNeedTracing(nsISupports* aThis);

  bool HasNothingToTrace(nsISupports* aThis);

  // Only meant to be called by code that preserves a wrapper.
  void SetPreservingWrapper(bool aPreserve)
  {
    if(aPreserve) {
      SetWrapperFlags(WRAPPER_BIT_PRESERVED);
    }
    else {
      UnsetWrapperFlags(WRAPPER_BIT_PRESERVED);
    }
  }

  void TraceWrapper(const TraceCallbacks& aCallbacks, void* aClosure)
  {
    if (PreservingWrapper() && mWrapper) {
      aCallbacks.Trace(&mWrapper, "Preserved wrapper", aClosure);
    }
  }

  /* 
   * The following methods for getting and manipulating flags allow the unused
   * bits of mFlags to be used by derived classes.
   */

  uint32_t GetFlags() const
  {
    return mFlags & ~kWrapperFlagsMask;
  }

  bool HasFlag(uint32_t aFlag) const
  {
    MOZ_ASSERT((aFlag & kWrapperFlagsMask) == 0, "Bad flag mask");
    return !!(mFlags & aFlag);
  }

  void SetFlags(uint32_t aFlagsToSet)
  {
    MOZ_ASSERT((aFlagsToSet & kWrapperFlagsMask) == 0, "Bad flag mask");
    mFlags |= aFlagsToSet;
  }

  void UnsetFlags(uint32_t aFlagsToUnset)
  {
    MOZ_ASSERT((aFlagsToUnset & kWrapperFlagsMask) == 0, "Bad flag mask");
    mFlags &= ~aFlagsToUnset;
  }

  void PreserveWrapper(nsISupports* aScriptObjectHolder)
  {
    if (PreservingWrapper()) {
      return;
    }

    nsISupports* ccISupports;
    aScriptObjectHolder->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                                        reinterpret_cast<void**>(&ccISupports));
    MOZ_ASSERT(ccISupports);

    nsXPCOMCycleCollectionParticipant* participant;
    CallQueryInterface(ccISupports, &participant);
    PreserveWrapper(ccISupports, participant);
  }

  void PreserveWrapper(void* aScriptObjectHolder, nsScriptObjectTracer* aTracer)
  {
    if (PreservingWrapper()) {
      return;
    }

    HoldJSObjects(aScriptObjectHolder, aTracer);
    SetPreservingWrapper(true);
#ifdef DEBUG
    // Make sure the cycle collector will be able to traverse to the wrapper.
    CheckCCWrapperTraversal(aScriptObjectHolder, aTracer);
#endif
  }

  void ReleaseWrapper(void* aScriptObjectHolder);

protected:
  void TraceWrapper(JSTracer* aTrc, const char* name)
  {
    if (mWrapper) {
      JS_CallHeapObjectTracer(aTrc, &mWrapper, name);
    }
  }

  void PoisonWrapper()
  {
    if (mWrapper) {
      mWrapper.setToCrashOnTouch();
    }
  }

private:
  JSObject *GetWrapperJSObject() const
  {
    return mWrapper;
  }

  void SetWrapperJSObject(JSObject* aWrapper)
  {
    mWrapper = aWrapper;
    UnsetWrapperFlags(kWrapperFlagsMask & ~WRAPPER_IS_DOM_BINDING);
  }

  void TraceWrapperJSObject(JSTracer* aTrc, const char* aName);

  uint32_t GetWrapperFlags() const
  {
    return mFlags & kWrapperFlagsMask;
  }

  bool HasWrapperFlag(uint32_t aFlag) const
  {
    MOZ_ASSERT((aFlag & ~kWrapperFlagsMask) == 0, "Bad wrapper flag bits");
    return !!(mFlags & aFlag);
  }

  void SetWrapperFlags(uint32_t aFlagsToSet)
  {
    MOZ_ASSERT((aFlagsToSet & ~kWrapperFlagsMask) == 0, "Bad wrapper flag bits");
    mFlags |= aFlagsToSet;
  }

  void UnsetWrapperFlags(uint32_t aFlagsToUnset)
  {
    MOZ_ASSERT((aFlagsToUnset & ~kWrapperFlagsMask) == 0, "Bad wrapper flag bits");
    mFlags &= ~aFlagsToUnset;
  }

  static void HoldJSObjects(void* aScriptObjectHolder,
                            nsScriptObjectTracer* aTracer);

#ifdef DEBUG
  void CheckCCWrapperTraversal(void* aScriptObjectHolder,
                               nsScriptObjectTracer* aTracer);
#endif // DEBUG

  /**
   * If this bit is set then we're preserving the wrapper, which in effect ties
   * the lifetime of the JS object stored in the cache to the lifetime of the
   * native object. We rely on the cycle collector to break the cycle that this
   * causes between the native object and the JS object, so it is important that
   * any native object that supports preserving of its wrapper
   * traces/traverses/unlinks the cached JS object (see
   * NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER,
   * NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS and
   * NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER).
   */
  enum { WRAPPER_BIT_PRESERVED = 1 << 0 };

  /**
   * If this bit is set then the wrapper for the native object is a DOM binding
   * (regular JS object or proxy).
   */
  enum { WRAPPER_IS_DOM_BINDING = 1 << 1 };

  enum { kWrapperFlagsMask = (WRAPPER_BIT_PRESERVED | WRAPPER_IS_DOM_BINDING) };

  JS::Heap<JSObject*> mWrapper;
  uint32_t            mFlags;
};

enum { WRAPPER_CACHE_FLAGS_BITS_USED = 2 };

NS_DEFINE_STATIC_IID_ACCESSOR(nsWrapperCache, NS_WRAPPERCACHE_IID)

#define NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY                                   \
  if ( aIID.Equals(NS_GET_IID(nsWrapperCache)) ) {                            \
    *aInstancePtr = static_cast<nsWrapperCache*>(this);                       \
    return NS_OK;                                                             \
  }


// Cycle collector macros for wrapper caches.

#define NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER \
  tmp->TraceWrapper(aCallbacks, aClosure);

#define NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER \
  tmp->ReleaseWrapper(p);

#define NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class) \
  NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(_class)              \
    NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER        \
  NS_IMPL_CYCLE_COLLECTION_TRACE_END

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(_class) \
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)         \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER   \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                   \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)       \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS    \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                 \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_CLASS(_class)                   \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(_class)            \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(__VA_ARGS__)           \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER      \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                      \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(_class)          \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(__VA_ARGS__)         \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS       \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END                    \
  NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(_class)

// TODO: Remove these after changing everything to the variadic variant.
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_2(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_3(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_4(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_5(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_6(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_7(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_8(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_9(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_10(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_11(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_12(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)
#define NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_13(_class, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(_class, __VA_ARGS__)

#endif /* nsWrapperCache_h___ */
