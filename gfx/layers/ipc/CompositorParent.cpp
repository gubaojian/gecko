/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set sw=2 ts=2 et tw=80 : */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Content App.
 *
 * The Initial Developer of the Original Code is
 *   The Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Benoit Girard <bgirard@mozilla.com>
 *   Ali Juma <ajuma@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "CompositorParent.h"
#include "ShadowLayersParent.h"
#include "LayerManagerOGL.h"
#include "nsIWidget.h"
#include "nsGkAtoms.h"
#include "RenderTrace.h"

#if defined(MOZ_WIDGET_ANDROID)
#include "AndroidBridge.h"
#include <android/log.h>
#endif

namespace mozilla {
namespace layers {

CompositorParent::CompositorParent(nsIWidget* aWidget)
  : mWidget(aWidget)
  , mCurrentCompositeTask(NULL)
  , mPaused(false)
{
  MOZ_COUNT_CTOR(CompositorParent);
}

CompositorParent::~CompositorParent()
{
  MOZ_COUNT_DTOR(CompositorParent);
}

void
CompositorParent::Destroy()
{
  NS_ABORT_IF_FALSE(ManagedPLayersParent().Length() == 0,
                    "CompositorParent destroyed before managed PLayersParent");

  // Ensure that the layer manager is destroyed on the compositor thread.
  mLayerManager = NULL;
}

bool
CompositorParent::RecvStop()
{
  mPaused = true;
  Destroy();
  return true;
}

void
CompositorParent::ScheduleRenderOnCompositorThread(::base::Thread &aCompositorThread)
{
  CancelableTask *renderTask = NewRunnableMethod(this, &CompositorParent::AsyncRender);
  aCompositorThread.message_loop()->PostTask(FROM_HERE, renderTask);
}

void
CompositorParent::PauseComposition()
{
  if (!mPaused) {
    mPaused = true;

#ifdef MOZ_WIDGET_ANDROID
    static_cast<LayerManagerOGL*>(mLayerManager.get())->gl()->ReleaseSurface();
#endif
  }
}

void
CompositorParent::ResumeComposition()
{
  mPaused = false;

#ifdef MOZ_WIDGET_ANDROID
  static_cast<LayerManagerOGL*>(mLayerManager.get())->gl()->RenewSurface();
#endif
}

void
CompositorParent::SchedulePauseOnCompositorThread(::base::Thread &aCompositorThread)
{
  CancelableTask *pauseTask = NewRunnableMethod(this,
                                                &CompositorParent::PauseComposition);
  aCompositorThread.message_loop()->PostTask(FROM_HERE, pauseTask);
}

void
CompositorParent::ScheduleResumeOnCompositorThread(::base::Thread &aCompositorThread)
{
  CancelableTask *resumeTask = NewRunnableMethod(this,
                                                 &CompositorParent::ResumeComposition);
  aCompositorThread.message_loop()->PostTask(FROM_HERE, resumeTask);
}

void
CompositorParent::ScheduleComposition()
{
  if (mCurrentCompositeTask) {
    return;
  }

  bool initialComposition = mLastCompose.IsNull();
  TimeDuration delta;
  if (!initialComposition)
    delta = mozilla::TimeStamp::Now() - mLastCompose;

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  mExpectedComposeTime = mozilla::TimeStamp::Now() + TimeDuration::FromMilliseconds(15);
#endif

  mCurrentCompositeTask = NewRunnableMethod(this, &CompositorParent::Composite);
  if (!initialComposition && delta.ToMilliseconds() < 15) {
#ifdef COMPOSITOR_PERFORMANCE_WARNING
    mExpectedComposeTime = mozilla::TimeStamp::Now() + TimeDuration::FromMilliseconds(15 - delta.ToMilliseconds());
#endif
    MessageLoop::current()->PostDelayedTask(FROM_HERE, mCurrentCompositeTask, 15 - delta.ToMilliseconds());
  } else {
    MessageLoop::current()->PostTask(FROM_HERE, mCurrentCompositeTask);
  }
}

void
CompositorParent::SetTransformation(float aScale, nsIntPoint aScrollOffset)
{
  mXScale = aScale;
  mYScale = aScale;
  mScrollOffset = aScrollOffset;
}

void
CompositorParent::Composite()
{
  mCurrentCompositeTask = NULL;

  mLastCompose = mozilla::TimeStamp::Now();

  if (mPaused || !mLayerManager) {
    return;
  }

#ifdef MOZ_WIDGET_ANDROID
  RequestViewTransform();
  TransformShadowTree();
#endif

  Layer* aLayer = mLayerManager->GetRoot();
  mozilla::layers::RenderTraceLayers(aLayer, "0000");

  mLayerManager->EndEmptyTransaction();

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  if (mExpectedComposeTime + TimeDuration::FromMilliseconds(15) < mozilla::TimeStamp::Now()) {
    printf_stderr("Compositor: Composite took %i ms.\n",
                  15 + (int)(mozilla::TimeStamp::Now() - mExpectedComposeTime).ToMilliseconds());
  }
#endif
}

#ifdef MOZ_WIDGET_ANDROID
// Do a breadth-first search to find the first layer in the tree that is
// scrollable.
Layer*
CompositorParent::GetPrimaryScrollableLayer()
{
  Layer* root = mLayerManager->GetRoot();

  nsTArray<Layer*> queue;
  queue.AppendElement(root);
  for (unsigned i = 0; i < queue.Length(); i++) {
    ContainerLayer* containerLayer = queue[i]->AsContainerLayer();
    if (!containerLayer) {
      continue;
    }

    const FrameMetrics& frameMetrics = containerLayer->GetFrameMetrics();
    if (frameMetrics.IsScrollable()) {
      return containerLayer;
    }

    Layer* child = containerLayer->GetFirstChild();
    while (child) {
      queue.AppendElement(child);
      child = child->GetNextSibling();
    }
  }

  return root;
}
#endif

// Go down shadow layer tree, setting properties to match their non-shadow
// counterparts.
static void
SetShadowProperties(Layer* aLayer)
{
  // FIXME: Bug 717688 -- Do these updates in ShadowLayersParent::RecvUpdate.
  ShadowLayer* shadow = aLayer->AsShadowLayer();
  shadow->SetShadowTransform(aLayer->GetTransform());
  shadow->SetShadowVisibleRegion(aLayer->GetVisibleRegion());
  shadow->SetShadowClipRect(aLayer->GetClipRect());

  for (Layer* child = aLayer->GetFirstChild();
      child; child = child->GetNextSibling()) {
    SetShadowProperties(child);
  }
}

static double GetXScale(const gfx3DMatrix& aTransform)
{
  return aTransform._11;
}

static double GetYScale(const gfx3DMatrix& aTransform)
{
  return aTransform._22;
}

void
CompositorParent::TransformShadowTree()
{
#ifdef MOZ_WIDGET_ANDROID
  Layer* layer = GetPrimaryScrollableLayer();
  ShadowLayer* shadow = layer->AsShadowLayer();

  gfx3DMatrix shadowTransform = layer->GetTransform();

  ContainerLayer* container = layer->AsContainerLayer();

  const FrameMetrics* metrics = &container->GetFrameMetrics();
  const gfx3DMatrix& currentTransform = layer->GetTransform();

  if (metrics && metrics->IsScrollable()) {
    float tempScaleDiffX = GetXScale(mLayerManager->GetRoot()->GetTransform()) * mXScale;
    float tempScaleDiffY = GetYScale(mLayerManager->GetRoot()->GetTransform()) * mYScale;

    nsIntPoint metricsScrollOffset = metrics->mViewportScrollOffset;

    nsIntPoint scrollCompensation(
      (mScrollOffset.x / tempScaleDiffX - metricsScrollOffset.x) * mXScale,
      (mScrollOffset.y / tempScaleDiffY - metricsScrollOffset.y) * mYScale);
    ViewTransform treeTransform(-scrollCompensation, mXScale,
                              mYScale);
    shadowTransform = gfx3DMatrix(treeTransform) * currentTransform;

    shadow->SetShadowTransform(shadowTransform);
  } else {
    ViewTransform treeTransform(nsIntPoint(0,0), mXScale,
                              mYScale);
    shadowTransform = gfx3DMatrix(treeTransform) * currentTransform;

    shadow->SetShadowTransform(shadowTransform);
  }
#endif
}

void
CompositorParent::AsyncRender()
{
  if (mPaused || !mLayerManager) {
    return;
  }

  Layer* root = mLayerManager->GetRoot();
  if (!root) {
    return;
  }

  ScheduleComposition();
}

#ifdef MOZ_WIDGET_ANDROID
void
CompositorParent::RequestViewTransform()
{
  mozilla::AndroidBridge::Bridge()->GetViewTransform(mScrollOffset, mXScale, mYScale);
}
#endif

void
CompositorParent::ShadowLayersUpdated()
{
  const nsTArray<PLayersParent*>& shadowParents = ManagedPLayersParent();
  NS_ABORT_IF_FALSE(shadowParents.Length() <= 1,
                    "can only support at most 1 ShadowLayersParent");
  if (shadowParents.Length()) {
    Layer* root = static_cast<ShadowLayersParent*>(shadowParents[0])->GetRoot();
    mLayerManager->SetRoot(root);
    SetShadowProperties(root);
  }
  ScheduleComposition();
}

PLayersParent*
CompositorParent::AllocPLayers(const LayersBackend &backendType)
{
  if (backendType == LayerManager::LAYERS_OPENGL) {
    nsRefPtr<LayerManagerOGL> layerManager = new LayerManagerOGL(mWidget);
    mWidget = NULL;
    mLayerManager = layerManager;

    if (!layerManager->Initialize()) {
      NS_ERROR("Failed to init OGL Layers");
      return NULL;
    }

    ShadowLayerManager* slm = layerManager->AsShadowManager();
    if (!slm) {
      return NULL;
    }
    return new ShadowLayersParent(slm, this);
  } else {
    NS_ERROR("Unsupported backend selected for Async Compositor");
    return NULL;
  }
}

bool
CompositorParent::DeallocPLayers(PLayersParent* actor)
{
  delete actor;
  return true;
}

} // namespace layers
} // namespace mozilla

