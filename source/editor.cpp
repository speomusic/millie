//#include "merc/editor.h"
//#include "merc/vst-utils.h"
//#include "chain-impl.h"
//
//namespace merc
//{
//    Steinberg::tresult Editor::beginEdit(Steinberg::Vst::ParamID id)
//    {
//        return Steinberg::kNotImplemented;
//    }
//
//    Steinberg::tresult Editor::performEdit(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized)
//    {
//        return Steinberg::kNotImplemented;
//    }
//
//    Steinberg::tresult Editor::endEdit(Steinberg::Vst::ParamID id)
//    {
//        return Steinberg::kNotImplemented;
//    }
//
//    Steinberg::tresult Editor::restartComponent(Steinberg::int32 flags)
//    {
//        return Steinberg::kNotImplemented;
//    }
//
//    Steinberg::IPtr<Steinberg::Vst::IEditController> Editor::makeEditController(const Steinberg::IPtr<Steinberg::Vst::IComponent>& component,
//                                                                                ChainImpl* chain,
//                                                                                Space& space)
//    {
//        if (const auto editController{ Steinberg::FUnknownPtr<Steinberg::Vst::IEditController>(component) })
//            return editController;
//        Steinberg::TUID editControllerID;
//        if (component->getControllerClassId(editControllerID) == Steinberg::kResultOk
//            && Steinberg::FUID{ editControllerID }.isValid())
//            if (const auto editController{ space.getEditor(editControllerID) })
//            {
//                CHECK_TRESULT(editController->initialize(chain));
//                return editController;
//            }
//        throw std::runtime_error{ "Asked for editor of a component without an edit controller." };
//    }
//
//    IMPLEMENT_FUNKNOWN_METHODS(Editor, Steinberg::Vst::IComponentHandler, Steinberg::Vst::IComponentHandler::iid)
//}
