

#ifndef RenderTreeAsText_h
#define RenderTreeAsText_h

namespace WebCore {

    class RenderObject;
    class String;
    class TextStream;

    String externalRepresentation(RenderObject*);
    void write(TextStream&, const RenderObject&, int indent = 0);

    // Helper function shared with SVGRenderTreeAsText
    String quoteAndEscapeNonPrintables(const String&);

} // namespace WebCore

#endif // RenderTreeAsText_h
