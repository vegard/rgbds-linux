#ifndef	ASMOTOR_LINK_MAPFILE_H
#define	ASMOTOR_LINK_MAPFILE_H

extern void SetMapfileName(char *name);
extern void SetSymfileName(char *name);
extern void CloseMapfile(void);
extern void MapfileWriteSection(struct sSection * pSect);
extern void MapfileInitBank(SLONG bank);
extern void MapfileCloseBank(SLONG slack);

#endif
