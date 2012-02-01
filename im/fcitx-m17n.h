#ifndef __FCITX_M17N_H
#define __FCITX_M17N_H

void *FcitxM17NCreate(FcitxInstance *instance);
void FcitxM17NDestroy(void *arg);

// CONFIG_BINDING_DECLARE(FcitxM17NConfig);

struct _Addon;
typedef struct {
	struct _Addon* owner;
	MInputMethod* mim;
	MInputContext* mic;
} IM;

typedef struct _Addon {
	FcitxInstance* owner;
	size_t nim;
	IM** ims;
} Addon;

#endif
