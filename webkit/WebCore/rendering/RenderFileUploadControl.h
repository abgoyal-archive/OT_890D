

#ifndef RenderFileUploadControl_h
#define RenderFileUploadControl_h

#include "FileChooser.h"
#include "RenderBlock.h"

namespace WebCore {

class HTMLInputElement;
    
// Each RenderFileUploadControl contains a RenderButton (for opening the file chooser), and
// sufficient space to draw a file icon and filename. The RenderButton has a shadow node
// associated with it to receive click/hover events.

class RenderFileUploadControl : public RenderBlock, private FileChooserClient {
public:
    RenderFileUploadControl(HTMLInputElement*);
    virtual ~RenderFileUploadControl();

    void click();

    void valueChanged();
    
    void receiveDroppedFiles(const Vector<String>&);

    String buttonValue();
    String fileTextValue();
    
    bool allowsMultipleFiles();

private:
    virtual const char* renderName() const { return "RenderFileUploadControl"; }

    virtual void updateFromElement();
    virtual void calcPrefWidths();
    virtual void paintObject(PaintInfo&, int tx, int ty);

    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

    int maxFilenameWidth() const;
    PassRefPtr<RenderStyle> createButtonStyle(const RenderStyle* parentStyle) const;

    RefPtr<HTMLInputElement> m_button;
    RefPtr<FileChooser> m_fileChooser;
};

inline RenderFileUploadControl* toRenderFileUploadControl(RenderObject* object)
{
    ASSERT(!object || !strcmp(object->renderName(), "RenderFileUploadControl"));
    return static_cast<RenderFileUploadControl*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderFileUploadControl(const RenderFileUploadControl*);

} // namespace WebCore

#endif // RenderFileUploadControl_h
