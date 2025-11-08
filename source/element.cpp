#include "merc/av/element.h"
#include "merc/av/merc-element.h"
#include "merc/av/merc-av.h"
#include "merc/vst/vst-utils.h"
#include "visitors.h"

namespace merc::av
{
    template<typename T>
    struct InMediumCreator
    {
        structure::Index index;
        const Merc& merc;

        InMedium<T> operator()(const structure::NextIn& nextIn)
        {
            const structure::Index previousIndex{ index.chain, index.element - 1 };
            const auto& previousMedia{ std::get<Work<Element<T>>>(merc.works).at(previousIndex).media };
            const auto& out{ previousMedia.outs.at(nextIn.previousOutIndex) };
            return &std::get<typename ElementTraits<T>::Resource>(out);
        }

        InMedium<T> operator()(const structure::ChainIn& chainIn)
        {
            const auto& chainOutMedia{ std::get<Work<Element<T>>>(merc.works).at(chainIn.index).media };
            const auto& out{ chainOutMedia.outs.at(chainIn.outIndex) };
            return &std::get<std::array<typename ElementTraits<T>::Resource, 2>>(out);
        }

        InMedium<T> operator()(const structure::MercIn& mercIn)
        {
            return mercIn.parentElementInIndex;
        }
    };

    template<typename T>
    struct OutMediumCreator
    {
        const Merc& merc;

        OutMedium<T> operator()(const typename ElementTraits<T>::NextOut& nextOut)
        {
            return createResource<T>(nextOut.arrangement, merc);
        }

        OutMedium<T> operator()(const typename ElementTraits<T>::ChainOut& chainOut)
        {
            return std::array
            {
                createResource<T>(chainOut.arrangement, merc),
                createResource<T>(chainOut.arrangement, merc)
            };
        }

        OutMedium<T> operator()(const structure::MercOut& mercOut)
        {
            return mercOut.parentElementOutIndex;
        }

        OutMedium<T> operator()(const structure::MainOut& mercOut)
        {
            return std::get<typename ElementRoutingTraits<T, structure::MainOut>::Medium>(merc.mainOut.revolvers);
        }
    };

    template<typename T>
    struct ArrangementGetter
    {
        structure::Index index;
        const Merc& merc;

        typename ElementTraits<T>::Arrangement operator()(const structure::NextIn& nextIn)
        {
            const structure::Index previousIndex{ index.chain, index.element - 1 };
            const auto& previousRoutings{ std::get<Work<Element<T>>>(merc.works).at(previousIndex).routing };
            const auto& out{ previousRoutings.outs.at(nextIn.previousOutIndex) };
            return ArrangementGetter{ previousIndex, merc }(std::get<typename ElementTraits<T>::NextOut>(out));
        }

        typename ElementTraits<T>::Arrangement operator()(const structure::ChainIn& chainIn)
        {
            const auto& chainOutRoutings{ std::get<Work<Element<T>>>(merc.works).at(chainIn.index).routing };
            const auto& out{ chainOutRoutings.outs.at(chainIn.outIndex) };
            return ArrangementGetter{ chainIn.index, merc }(std::get<typename ElementTraits<T>::ChainOut>(out));
        }

        typename ElementTraits<T>::Arrangement operator()(const structure::MercIn& mercIn)
        {
            const auto& parent{ merc.parent.value() };
            const auto& parentRoutings{ std::get<Work<Element<T>>>(parent.merc.works).at(parent.index).routing };
            const auto& in{ parentRoutings.ins.at(mercIn.parentElementInIndex) };
            return std::visit(ArrangementGetter<T>{ parent.index, parent.merc }, in);
        }

        typename ElementTraits<T>::Arrangement operator()(const typename ElementTraits<T>::NextOut& nextOut)
        {
            return nextOut.arrangement;
        }

        typename ElementTraits<T>::Arrangement operator()(const typename ElementTraits<T>::ChainOut& chainOut)
        {
            return chainOut.arrangement;
        }

        typename ElementTraits<T>::Arrangement operator()(const structure::MercOut& mercOut)
        {
            const auto& parent{ merc.parent.value() };
            const auto& parentRoutings{ std::get<Work<Element<T>>>(parent.merc.works).at(parent.index).routing };
            const auto& out{ parentRoutings.outs.at(mercOut.parentElementOutIndex) };
            return std::visit(ArrangementGetter<T>{ parent.index, parent.merc }, out);
        }

        typename ElementTraits<T>::Arrangement operator()(const structure::MainOut& mainOut)
        {
            return getMainOutArrangement<T>();
        }
    };

    template<typename T>
    Media<T> createMedia(const typename ElementTraits<T>::Routing& routing, structure::Index index, const Merc& merc)
    {
        Media<T> media;
        media.ins.reserve(routing.ins.size());
        for (const auto& in : routing.ins)
            media.ins.push_back(std::visit(InMediumCreator<T>{ index, merc }, in));
        media.outs.reserve(routing.outs.size());
        for (const auto& out : routing.outs)
            media.outs.push_back(std::visit(OutMediumCreator<T>{ merc }, out));
        return media;
    }

    template<typename T>
    Element<T>::Element(structure::Runner runner,
                        typename ElementTraits<T>::Routing rt,
                        structure::Index ind,
                        Merc& m)
        : routing{ std::move(rt) }
        , index{ ind }
        , merc{ m }
        , media{ createMedia<T>(routing, index, merc) }
        , impl{ std::visit([&](auto r)
        {
            return std::variant<VstImpl, MercImpl>{ std::make_unique<ElementImplementation<T, decltype(r)>>(*this, std::move(r)) };
        }, std::move(runner)) }
    {}

    template<typename T>
    void Element<T>::run(TimeKeeper<Element<T>>& timeKeeper)
    {
        std::visit([&](auto& i){ i->run(timeKeeper); }, impl);
    }

    template<typename T>
    void Element<T>::onRoutingAndMediaChanged()
    {
        std::visit([](auto& i){ i->onRoutingAndMediaChanged(); }, impl);
    }

    template<typename T>
    bool Element<T>::hasMercInOrOutRouting() const
    {
        for (const auto& in : routing.ins)
            if (std::holds_alternative<structure::MercIn>(in))
                return true;
        for (const auto& out : routing.outs)
            if (std::holds_alternative<structure::MercOut>(out))
                return true;
        return false;
    }

    template<typename T>
    void Element<T>::changeRouting(typename ElementTraits<T>::Routing newRouting)
    {
        routing = std::move(newRouting);
        media = createMedia<T>(routing, index, merc);
        onRoutingAndMediaChanged();
    }

    template<typename T>
    void Element<T>::pushEvent(TimedEvent event)
    {
        std::visit([=](auto& i){ i->pushEvent(event); }, impl);
    }

    template<typename T>
    std::vector<typename ElementTraits<T>::Arrangement> Element<T>::getInArrangements() const
    {
        std::vector<typename ElementTraits<T>::Arrangement> inArrangements; inArrangements.reserve(routing.ins.size());
        for (const auto& in : routing.ins) inArrangements.push_back(std::visit(ArrangementGetter<T>{ index, merc }, in));
        return inArrangements;
    }

    template<typename T>
    std::vector<typename ElementTraits<T>::Arrangement> Element<T>::getOutArrangements() const
    {
        std::vector<typename ElementTraits<T>::Arrangement> outArrangements; outArrangements.reserve(routing.outs.size());
        for (const auto& out : routing.outs) outArrangements.push_back(std::visit(ArrangementGetter<T>{ index, merc }, out));
        return outArrangements;
    }

    template struct Element<structure::Link>;
    template struct Element<structure::Step>;
}
