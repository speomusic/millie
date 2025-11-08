#pragma once

#include "merc/space.h"
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <memory>

namespace merc
{
    struct Editor : Steinberg::Vst::IComponentHandler
    {
        Editor(const Steinberg::IPtr<Steinberg::Vst::IComponent>& component,
               std::shared_ptr<struct ChainImpl> chain,
               Space& space);
        ~Editor();
        operator bool() const;

        Steinberg::tresult PLUGIN_API beginEdit(Steinberg::Vst::ParamID id) override;
        Steinberg::tresult PLUGIN_API performEdit(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized) override;
        Steinberg::tresult PLUGIN_API endEdit(Steinberg::Vst::ParamID id) override;
        Steinberg::tresult PLUGIN_API restartComponent(Steinberg::int32 flags) override;
        DECLARE_FUNKNOWN_METHODS
    private:
        std::shared_ptr<struct ChainImpl> chain;
        Steinberg::IPtr<Steinberg::Vst::IEditController> editController;
        const bool editControllerIsSeparatePluginBase;
        std::unique_ptr<struct EditorWindow> window;

        Steinberg::IPtr<Steinberg::Vst::IEditController> makeEditController(const Steinberg::IPtr<Steinberg::Vst::IComponent>& component,
                                                                            ChainImpl* chain,
                                                                            Space& space);
    };
}
