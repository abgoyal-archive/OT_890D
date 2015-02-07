

#ifndef SVGListTraits_h
#define SVGListTraits_h

#if ENABLE(SVG)

namespace WebCore {

    template<typename Item> struct UsesDefaultInitializer { static const bool value = true; };
    template<> struct UsesDefaultInitializer<double>      { static const bool value = false; };

    template<bool usesDefaultInitializer, typename Item>
    struct SVGListTraits { };

    template<typename Item>
    struct SVGListTraits<true, Item> {
        static Item nullItem() { return Item(); } 
    };

    template<>
    struct SVGListTraits<false, double> {
        static double nullItem() { return 0.0; }
    };

} // namespace WebCore

#endif // SVG_SUPPORT
#endif // SVGListTraits_h

// vim:ts=4:noet
