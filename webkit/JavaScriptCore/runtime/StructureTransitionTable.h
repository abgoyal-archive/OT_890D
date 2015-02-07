

#ifndef StructureTransitionTable_h
#define StructureTransitionTable_h

#include "UString.h"
#include <wtf/HashFunctions.h>
#include <wtf/HashMap.h>
#include <wtf/HashTraits.h>
#include <wtf/RefPtr.h>

namespace JSC {

    class Structure;

    struct StructureTransitionTableHash {
        typedef std::pair<RefPtr<UString::Rep>, std::pair<unsigned, JSCell*> > Key;
        static unsigned hash(const Key& p)
        {
            return p.first->computedHash();
        }

        static bool equal(const Key& a, const Key& b)
        {
            return a == b;
        }

        static const bool safeToCompareToEmptyOrDeleted = true;
    };

    struct StructureTransitionTableHashTraits {
        typedef WTF::HashTraits<RefPtr<UString::Rep> > FirstTraits;
        typedef WTF::GenericHashTraits<unsigned> SecondFirstTraits;
        typedef WTF::GenericHashTraits<JSCell*> SecondSecondTraits;
        typedef std::pair<FirstTraits::TraitType, std::pair<SecondFirstTraits::TraitType, SecondSecondTraits::TraitType> > TraitType;

        static const bool emptyValueIsZero = FirstTraits::emptyValueIsZero && SecondFirstTraits::emptyValueIsZero && SecondSecondTraits::emptyValueIsZero;
        static TraitType emptyValue() { return std::make_pair(FirstTraits::emptyValue(), std::make_pair(SecondFirstTraits::emptyValue(), SecondSecondTraits::emptyValue())); }

        static const bool needsDestruction = FirstTraits::needsDestruction || SecondFirstTraits::needsDestruction || SecondSecondTraits::needsDestruction;

        static void constructDeletedValue(TraitType& slot) { FirstTraits::constructDeletedValue(slot.first); }
        static bool isDeletedValue(const TraitType& value) { return FirstTraits::isDeletedValue(value.first); }
    };

    typedef HashMap<StructureTransitionTableHash::Key, Structure*, StructureTransitionTableHash, StructureTransitionTableHashTraits> StructureTransitionTable;

} // namespace JSC

#endif // StructureTransitionTable_h
