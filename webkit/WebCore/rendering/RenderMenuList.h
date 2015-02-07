

#ifndef RenderMenuList_h
#define RenderMenuList_h

#include "PopupMenuClient.h"
#include "RenderFlexibleBox.h"

#if PLATFORM(MAC)
#define POPUP_MENU_PULLS_DOWN 0
#else
#define POPUP_MENU_PULLS_DOWN 1
#endif

namespace WebCore {

class PopupMenu;
class RenderText;

class RenderMenuList : public RenderFlexibleBox, private PopupMenuClient {
public:
    RenderMenuList(Element*);
    virtual ~RenderMenuList();

public:
    bool popupIsVisible() const { return m_popupIsVisible; }
    void showPopup();
    void hidePopup();

    void setOptionsChanged(bool changed) { m_optionsChanged = changed; }

    String text() const;

private:
    virtual bool isMenuList() const { return true; }

    virtual void addChild(RenderObject* newChild, RenderObject* beforeChild = 0);
    virtual void removeChild(RenderObject*);
    virtual bool createsAnonymousWrapper() const { return true; }
    virtual bool canHaveChildren() const { return false; }

    virtual void updateFromElement();

    virtual bool hasControlClip() const { return true; }
    virtual IntRect controlClipRect(int tx, int ty) const;

    virtual const char* renderName() const { return "RenderMenuList"; }

    virtual void calcPrefWidths();

    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

    // PopupMenuClient methods
    virtual String itemText(unsigned listIndex) const;
    virtual String itemToolTip(unsigned listIndex) const;
    virtual bool itemIsEnabled(unsigned listIndex) const;
    virtual PopupMenuStyle itemStyle(unsigned listIndex) const;
    virtual PopupMenuStyle menuStyle() const;
    virtual int clientInsetLeft() const;
    virtual int clientInsetRight() const;
    virtual int clientPaddingLeft() const;
    virtual int clientPaddingRight() const;
    virtual int listSize() const;
    virtual int selectedIndex() const;
    virtual bool itemIsSeparator(unsigned listIndex) const;
    virtual bool itemIsLabel(unsigned listIndex) const;
    virtual bool itemIsSelected(unsigned listIndex) const;
    virtual void setTextFromItem(unsigned listIndex);
    virtual bool valueShouldChangeOnHotTrack() const { return true; }
    virtual bool shouldPopOver() const { return !POPUP_MENU_PULLS_DOWN; }
    virtual void valueChanged(unsigned listIndex, bool fireOnChange = true);
    virtual FontSelector* fontSelector() const;
    virtual HostWindow* hostWindow() const;
    virtual PassRefPtr<Scrollbar> createScrollbar(ScrollbarClient*, ScrollbarOrientation, ScrollbarControlSize);

    virtual bool hasLineIfEmpty() const { return true; }

    Color itemBackgroundColor(unsigned listIndex) const;

    void createInnerBlock();
    void adjustInnerStyle();
    void setText(const String&);
    void setTextFromOption(int optionIndex);
    void updateOptionsWidth();

    RenderText* m_buttonText;
    RenderBlock* m_innerBlock;

    bool m_optionsChanged;
    int m_optionsWidth;

    RefPtr<PopupMenu> m_popup;
    bool m_popupIsVisible;
};

inline RenderMenuList* toRenderMenuList(RenderObject* object)
{ 
    ASSERT(!object || object->isMenuList());
    return static_cast<RenderMenuList*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderMenuList(const RenderMenuList*);

}

#endif