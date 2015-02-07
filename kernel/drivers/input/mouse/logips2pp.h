

#ifndef _LOGIPS2PP_H
#define _LOGIPS2PP_H

#ifdef CONFIG_MOUSE_PS2_LOGIPS2PP
int ps2pp_init(struct psmouse *psmouse, int set_properties);
#else
inline int ps2pp_init(struct psmouse *psmouse, int set_properties)
{
	return -ENOSYS;
}
#endif /* CONFIG_MOUSE_PS2_LOGIPS2PP */

#endif
