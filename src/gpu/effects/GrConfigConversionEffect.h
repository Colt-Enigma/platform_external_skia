/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**************************************************************************************************
 *** This file was autogenerated from GrConfigConversionEffect.fp; do not modify.
 **************************************************************************************************/
#ifndef GrConfigConversionEffect_DEFINED
#define GrConfigConversionEffect_DEFINED
#include "SkTypes.h"
#if SK_SUPPORT_GPU

#include "GrClip.h"
#include "GrContext.h"
#include "GrContextPriv.h"
#include "GrProxyProvider.h"
#include "GrRenderTargetContext.h"
#include "GrFragmentProcessor.h"
#include "GrCoordTransform.h"
class GrConfigConversionEffect : public GrFragmentProcessor {
public:
    static bool TestForPreservingPMConversions(GrContext* context) {
        static constexpr int kSize = 256;
        static constexpr GrPixelConfig kConfig = kRGBA_8888_GrPixelConfig;
        SkAutoTMalloc<uint32_t> data(kSize * kSize * 3);
        uint32_t* srcData = data.get();
        uint32_t* firstRead = data.get() + kSize * kSize;
        uint32_t* secondRead = data.get() + 2 * kSize * kSize;

        // Fill with every possible premultiplied A, color channel value. There will be 256-y
        // duplicate values in row y. We set r, g, and b to the same value since they are handled
        // identically.
        for (int y = 0; y < kSize; ++y) {
            for (int x = 0; x < kSize; ++x) {
                uint8_t* color = reinterpret_cast<uint8_t*>(&srcData[kSize * y + x]);
                color[3] = y;
                color[2] = SkTMin(x, y);
                color[1] = SkTMin(x, y);
                color[0] = SkTMin(x, y);
            }
        }

        const SkImageInfo ii =
                SkImageInfo::Make(kSize, kSize, kRGBA_8888_SkColorType, kPremul_SkAlphaType);

        sk_sp<GrRenderTargetContext> readRTC(context->makeDeferredRenderTargetContext(
                SkBackingFit::kExact, kSize, kSize, kConfig, nullptr));
        sk_sp<GrRenderTargetContext> tempRTC(context->makeDeferredRenderTargetContext(
                SkBackingFit::kExact, kSize, kSize, kConfig, nullptr));
        if (!readRTC || !readRTC->asTextureProxy() || !tempRTC) {
            return false;
        }
        // Adding discard to appease vulkan validation warning about loading uninitialized data on
        // draw
        readRTC->discard();

        GrSurfaceDesc desc;
        desc.fOrigin = kTopLeft_GrSurfaceOrigin;
        desc.fWidth = kSize;
        desc.fHeight = kSize;
        desc.fConfig = kConfig;

        GrProxyProvider* proxyProvider = context->contextPriv().proxyProvider();

        sk_sp<GrTextureProxy> dataProxy =
                proxyProvider->createTextureProxy(desc, SkBudgeted::kYes, data, 0);
        if (!dataProxy) {
            return false;
        }

        static const SkRect kRect = SkRect::MakeIWH(kSize, kSize);

        // We do a PM->UPM draw from dataTex to readTex and read the data. Then we do a UPM->PM draw
        // from readTex to tempTex followed by a PM->UPM draw to readTex and finally read the data.
        // We then verify that two reads produced the same values.

        GrPaint paint1;
        GrPaint paint2;
        GrPaint paint3;
        std::unique_ptr<GrFragmentProcessor> pmToUPM(
                new GrConfigConversionEffect(PMConversion::kToUnpremul));
        std::unique_ptr<GrFragmentProcessor> upmToPM(
                new GrConfigConversionEffect(PMConversion::kToPremul));

        paint1.addColorTextureProcessor(dataProxy, SkMatrix::I());
        paint1.addColorFragmentProcessor(pmToUPM->clone());
        paint1.setPorterDuffXPFactory(SkBlendMode::kSrc);

        readRTC->fillRectToRect(GrNoClip(), std::move(paint1), GrAA::kNo, SkMatrix::I(), kRect,
                                kRect);
        if (!readRTC->readPixels(ii, firstRead, 0, 0, 0)) {
            return false;
        }

        // Adding discard to appease vulkan validation warning about loading uninitialized data on
        // draw
        tempRTC->discard();

        paint2.addColorTextureProcessor(readRTC->asTextureProxyRef(), SkMatrix::I());
        paint2.addColorFragmentProcessor(std::move(upmToPM));
        paint2.setPorterDuffXPFactory(SkBlendMode::kSrc);

        tempRTC->fillRectToRect(GrNoClip(), std::move(paint2), GrAA::kNo, SkMatrix::I(), kRect,
                                kRect);

        paint3.addColorTextureProcessor(tempRTC->asTextureProxyRef(), SkMatrix::I());
        paint3.addColorFragmentProcessor(std::move(pmToUPM));
        paint3.setPorterDuffXPFactory(SkBlendMode::kSrc);

        readRTC->fillRectToRect(GrNoClip(), std::move(paint3), GrAA::kNo, SkMatrix::I(), kRect,
                                kRect);

        if (!readRTC->readPixels(ii, secondRead, 0, 0, 0)) {
            return false;
        }

        for (int y = 0; y < kSize; ++y) {
            for (int x = 0; x <= y; ++x) {
                if (firstRead[kSize * y + x] != secondRead[kSize * y + x]) {
                    return false;
                }
            }
        }

        return true;
    }
    PMConversion pmConversion() const { return fPmConversion; }

    static std::unique_ptr<GrFragmentProcessor> Make(std::unique_ptr<GrFragmentProcessor> fp,
                                                     PMConversion pmConversion) {
        if (!fp) {
            return nullptr;
        }
        std::unique_ptr<GrFragmentProcessor> ccFP(new GrConfigConversionEffect(pmConversion));
        std::unique_ptr<GrFragmentProcessor> fpPipeline[] = {std::move(fp), std::move(ccFP)};
        return GrFragmentProcessor::RunInSeries(fpPipeline, 2);
    }
    GrConfigConversionEffect(const GrConfigConversionEffect& src);
    std::unique_ptr<GrFragmentProcessor> clone() const override;
    const char* name() const override { return "ConfigConversionEffect"; }

private:
    GrConfigConversionEffect(PMConversion pmConversion)
            : INHERITED(kGrConfigConversionEffect_ClassID, kNone_OptimizationFlags)
            , fPmConversion(pmConversion) {}
    GrGLSLFragmentProcessor* onCreateGLSLInstance() const override;
    void onGetGLSLProcessorKey(const GrShaderCaps&, GrProcessorKeyBuilder*) const override;
    bool onIsEqual(const GrFragmentProcessor&) const override;
    GR_DECLARE_FRAGMENT_PROCESSOR_TEST
    PMConversion fPmConversion;
    typedef GrFragmentProcessor INHERITED;
};
#endif
#endif
