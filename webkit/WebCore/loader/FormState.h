

#ifndef FormState_h
#define FormState_h

#include "PlatformString.h"

namespace WebCore {

    class Frame;
    class HTMLFormElement;

    typedef Vector<std::pair<String, String> > StringPairVector;

    class FormState : public RefCounted<FormState> {
    public:
        static PassRefPtr<FormState> create(PassRefPtr<HTMLFormElement>, StringPairVector& textFieldValuesToAdopt, PassRefPtr<Frame>);

        HTMLFormElement* form() const { return m_form.get(); }
        const StringPairVector& textFieldValues() const { return m_textFieldValues; }
        Frame* sourceFrame() const { return m_sourceFrame.get(); }

    private:
        FormState(PassRefPtr<HTMLFormElement>, StringPairVector& textFieldValuesToAdopt, PassRefPtr<Frame>);

        RefPtr<HTMLFormElement> m_form;
        StringPairVector m_textFieldValues;
        RefPtr<Frame> m_sourceFrame;
    };

}

#endif // FormState_h
