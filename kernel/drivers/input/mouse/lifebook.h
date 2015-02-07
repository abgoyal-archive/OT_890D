

#ifndef _LIFEBOOK_H
#define _LIFEBOOK_H

#ifdef CONFIG_MOUSE_PS2_LIFEBOOK
int lifebook_detect(struct psmouse *psmouse, int set_properties);
int lifebook_init(struct psmouse *psmouse);
#else
inline int lifebook_detect(struct psmouse *psmouse, int set_properties)
{
	return -ENOSYS;
}
inline int lifebook_init(struct psmouse *psmouse)
{
	return -ENOSYS;
}
#endif

#endif
