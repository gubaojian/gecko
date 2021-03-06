/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "PluginWidgetProxy.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/plugins/PluginWidgetChild.h"
#include "nsDebug.h"

#define PWLOG(...)
// #define PWLOG(...) printf_stderr(__VA_ARGS__)

/* static */
already_AddRefed<nsIWidget>
nsIWidget::CreatePluginProxyWidget(TabChild* aTabChild,
                                   mozilla::plugins::PluginWidgetChild* aActor)
{
  nsCOMPtr<nsIWidget> widget =
    new mozilla::widget::PluginWidgetProxy(aTabChild, aActor);
  return widget.forget();
}

namespace mozilla {
namespace widget {

NS_IMPL_ISUPPORTS_INHERITED(PluginWidgetProxy, PuppetWidget, nsIWidget)

#define ENSURE_CHANNEL do {                                   \
  if (!mActor) {                                              \
    NS_WARNING("called on an invalid channel.");              \
    return NS_ERROR_FAILURE;                                  \
  }                                                           \
} while (0)

PluginWidgetProxy::PluginWidgetProxy(dom::TabChild* aTabChild,
                                     mozilla::plugins::PluginWidgetChild* aActor) :
  PuppetWidget(aTabChild),
  mActor(aActor)
{
  // See ChannelDestroyed() in the header
  mActor->mWidget = this;
}

PluginWidgetProxy::~PluginWidgetProxy()
{
  PWLOG("PluginWidgetProxy::~PluginWidgetProxy()\n");
}

NS_IMETHODIMP
PluginWidgetProxy::Create(nsIWidget*        aParent,
                          nsNativeWidget    aNativeParent,
                          const nsIntRect&  aRect,
                          nsDeviceContext*  aContext,
                          nsWidgetInitData* aInitData)
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetProxy::Create()\n");

  if (!mActor->SendCreate()) {
    NS_WARNING("failed to create chrome widget, plugins won't paint.");
  }

  BaseCreate(aParent, aRect, aContext, aInitData);

  mBounds = aRect;
  mEnabled = true;
  mVisible = true;

  return NS_OK;
}

NS_IMETHODIMP
PluginWidgetProxy::SetParent(nsIWidget* aNewParent)
{
  mParent = aNewParent;

  nsCOMPtr<nsIWidget> kungFuDeathGrip(this);
  nsIWidget* parent = GetParent();
  if (parent) {
    parent->RemoveChild(this);
  }
  if (aNewParent) {
    aNewParent->AddChild(this);
  }
  return NS_OK;
}

nsIWidget*
PluginWidgetProxy::GetParent(void)
{
  return mParent.get();
}

NS_IMETHODIMP
PluginWidgetProxy::Destroy()
{
  PWLOG("PluginWidgetProxy::Destroy()\n");

  if (mActor) {
   /**
    * We need to communicate that the sub protocol is going to be torn down
    * before the sub protocol dies. Otherwise we can end up with async events
    * in transit from chrome to content, which on arrival will trigger an abort
    * in the content process, crashing all tabs.
    *
    * Note, this is one of two ways PluginWidget tear down initiates. Here we
    * are a plugin in content and content has just unloaded us for some reason,
    * usually due to swap out for flash ads or the user simply loaded a
    * different page. The other involves a full tear down of the tab (PBrowser)
    * which happens prior to widgets getting collected by ref counting in
    * layout. We still get this Destroy call, but in all likelyhood mActor is
    * already null via a call on ChannelDestroyed from PluginWidgetChild.
    */
    mActor->SendDestroy();
    mActor->mWidget = nullptr;
    mActor = nullptr;
  }

  return PuppetWidget::Destroy();
}

void
PluginWidgetProxy::GetWindowClipRegion(nsTArray<nsIntRect>* aRects)
{
  if (mClipRects && mClipRectCount) {
    aRects->AppendElements(mClipRects.get(), mClipRectCount);
  }
}

void*
PluginWidgetProxy::GetNativeData(uint32_t aDataType)
{
  if (!mActor) {
    return nullptr;
  }
  switch (aDataType) {
    case NS_NATIVE_PLUGIN_PORT:
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_SHAREABLE_WINDOW:
      break;
    default:
      NS_WARNING("PluginWidgetProxy::GetNativeData received request for unsupported data type.");
      return nullptr;
  }
  uintptr_t value = 0;
  mActor->SendGetNativePluginPort(&value);
  PWLOG("PluginWidgetProxy::GetNativeData %p\n", (void*)value);
  return (void*)value;
}

NS_IMETHODIMP
PluginWidgetProxy::SetFocus(bool aRaise)
{
  ENSURE_CHANNEL;
  PWLOG("PluginWidgetProxy::SetFocus(%d)\n", aRaise);
  mActor->SendSetFocus(aRaise);
  return NS_OK;
}

}  // namespace widget
}  // namespace mozilla
