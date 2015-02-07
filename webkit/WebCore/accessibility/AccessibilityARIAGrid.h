

#ifndef AccessibilityARIAGrid_h
#define AccessibilityARIAGrid_h

#include "AccessibilityTable.h"

namespace WebCore {
    
class String;
class AccessibilityTableCell;
class AccessibilityTableHeaderContainer;

class AccessibilityARIAGrid : public AccessibilityTable {
    
private:
    AccessibilityARIAGrid(RenderObject*);
public:
    static PassRefPtr<AccessibilityARIAGrid> create(RenderObject*);
    virtual ~AccessibilityARIAGrid();
    
    virtual bool isAriaTable() const { return true; }    
    
    virtual void addChildren();
    
    virtual AccessibilityTableCell* cellForColumnAndRow(unsigned column, unsigned row);

private:
    void addChild(AccessibilityObject* object, HashSet<AccessibilityObject*>& appendedRows, unsigned& columnCount);
};

} // namespace WebCore 

#endif // AccessibilityARIAGrid_h
