

#ifndef ValidityState_h
#define ValidityState_h

#include "HTMLFormControlElement.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

    class ValidityState : public RefCounted<ValidityState> {
    public:
        static PassRefPtr<ValidityState> create(HTMLFormControlElement* owner)
        {
            return adoptRef(new ValidityState(owner));
        }

        HTMLFormControlElement* control() const { return m_control; }

        void setCustomErrorMessage(const String& message) { m_customErrorMessage = message; }

        bool valueMissing() { return control()->valueMissing(); }
        bool typeMismatch() { return false; }
        bool patternMismatch() { return control()->patternMismatch(); }
        bool tooLong() { return false; }
        bool rangeUnderflow() { return false; }
        bool rangeOverflow() { return false; }
        bool stepMismatch() { return false; }
        bool customError() { return !m_customErrorMessage.isEmpty(); }
        bool valid();

    private:
        ValidityState(HTMLFormControlElement*);
        HTMLFormControlElement* m_control;
        String m_customErrorMessage;
    };

} // namespace WebCore

#endif // ValidityState_h
