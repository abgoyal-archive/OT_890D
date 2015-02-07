

#ifndef HTMLFormControlElement_h
#define HTMLFormControlElement_h

#include "HTMLElement.h"

namespace WebCore {

class FormDataList;
class HTMLFormElement;
class ValidityState;

class HTMLFormControlElement : public HTMLElement {
public:
    HTMLFormControlElement(const QualifiedName& tagName, Document*, HTMLFormElement*);
    virtual ~HTMLFormControlElement();

    virtual HTMLTagStatus endTagRequirement() const { return TagStatusRequired; }
    virtual int tagPriority() const { return 1; }

    HTMLFormElement* form() const { return m_form; }
    virtual ValidityState* validity();

    virtual bool isTextFormControl() const { return false; }
    virtual bool isEnabledFormControl() const { return !disabled(); }

    virtual void parseMappedAttribute(MappedAttribute*);
    virtual void attach();
    virtual void insertedIntoTree(bool deep);
    virtual void removedFromTree(bool deep);

    virtual void reset() {}

    virtual bool formControlValueMatchesRenderer() const { return m_valueMatchesRenderer; }
    virtual void setFormControlValueMatchesRenderer(bool b) { m_valueMatchesRenderer = b; }

    virtual void dispatchFormControlChangeEvent();

    bool disabled() const;
    void setDisabled(bool);

    virtual bool supportsFocus() const;
    virtual bool isFocusable() const;
    virtual bool isKeyboardFocusable(KeyboardEvent*) const;
    virtual bool isMouseFocusable() const;
    virtual bool isEnumeratable() const { return false; }

    virtual bool isReadOnlyFormControl() const { return m_readOnly; }
    void setReadOnly(bool);

    // Determines whether or not a control will be automatically focused
    virtual bool autofocus() const;
    void setAutofocus(bool);

    bool required() const;
    void setRequired(bool);

    virtual void recalcStyle(StyleChange);

    virtual const AtomicString& formControlName() const;
    virtual const AtomicString& formControlType() const = 0;

    const AtomicString& type() const { return formControlType(); }
    const AtomicString& name() const { return formControlName(); }

    void setName(const AtomicString& name);

    virtual bool isFormControlElement() const { return true; }
    virtual bool isRadioButton() const { return false; }

    /* Override in derived classes to get the encoded name=value pair for submitting.
     * Return true for a successful control (see HTML4-17.13.2).
     */
    virtual bool appendFormData(FormDataList&, bool) { return false; }

    virtual bool isSuccessfulSubmitButton() const { return false; }
    virtual bool isActivatedSubmit() const { return false; }
    virtual void setActivatedSubmit(bool) { }

    virtual short tabIndex() const;

    virtual bool willValidate() const;
    void setCustomValidity(const String&);

    virtual bool valueMissing() const { return false; }
    virtual bool patternMismatch() const { return false; }

    void formDestroyed() { m_form = 0; }

    virtual void dispatchFocusEvent();
    virtual void dispatchBlurEvent();

protected:
    void removeFromForm();

private:
    virtual HTMLFormElement* virtualForm() const;

    HTMLFormElement* m_form;
    RefPtr<ValidityState> m_validityState;
    bool m_disabled;
    bool m_readOnly;
    bool m_valueMatchesRenderer;
};

class HTMLFormControlElementWithState : public HTMLFormControlElement {
public:
    HTMLFormControlElementWithState(const QualifiedName& tagName, Document*, HTMLFormElement*);
    virtual ~HTMLFormControlElementWithState();

    virtual void finishParsingChildren();

protected:
    virtual void willMoveToNewOwnerDocument();
    virtual void didMoveToNewOwnerDocument();
};

} //namespace

#endif
