#include <stdio.h>
#include <stdlib.h>

#include "link/mylink.h"
#include "link/main.h"
#include "link/symbol.h"
#include "link/assign.h"

struct sFreeArea {
	SLONG nOrg;
	SLONG nSize;
	struct sFreeArea *pPrev, *pNext;
};

struct sFreeArea *BankFree[MAXBANKS];
SLONG MaxAvail[MAXBANKS];
SLONG MaxBankUsed;

#define DOMAXBANK(x)	{if( (x)>MaxBankUsed ) MaxBankUsed=(x);}

SLONG 
area_Avail(SLONG bank)
{
	SLONG r;
	struct sFreeArea *pArea;

	r = 0;
	pArea = BankFree[bank];

	while (pArea) {
		r += pArea->nSize;
		pArea = pArea->pNext;
	}

	return (r);
}

SLONG 
area_AllocAbs(struct sFreeArea ** ppArea, SLONG org, SLONG size)
{
	struct sFreeArea *pArea;

	pArea = *ppArea;
	while (pArea) {
		if (org >= pArea->nOrg
		    && (org + size - 1) <= (pArea->nOrg + pArea->nSize - 1)) {
			if (org == pArea->nOrg) {
				pArea->nOrg += size;
				pArea->nSize -= size;
				return (org);
			} else {
				if ((org + size - 1) ==
				    (pArea->nOrg + pArea->nSize - 1)) {
					pArea->nSize -= size;
					return (org);
				} else {
					struct sFreeArea *pNewArea;

					if ((pNewArea =
						(struct sFreeArea *)
						malloc(sizeof(struct sFreeArea)))
					    != NULL) {
						*pNewArea = *pArea;
						pNewArea->pPrev = pArea;
						pArea->pNext = pNewArea;
						pArea->nSize =
						    org - pArea->nOrg;
						pNewArea->nOrg = org + size;
						pNewArea->nSize -=
						    size + pArea->nSize;

						return (org);
					} else {
						fprintf(stderr,
						    "Out of memory!\n");
						exit(1);
					}
				}
			}
		}
		ppArea = &(pArea->pNext);
		pArea = *ppArea;
	}

	return (-1);
}

SLONG 
area_AllocAbsCODEAnyBank(SLONG org, SLONG size)
{
	SLONG i;

	for (i = 1; i <= 255; i += 1) {
		if (area_AllocAbs(&BankFree[i], org, size) == org)
			return (i);
	}

	return (-1);
}

SLONG 
area_Alloc(struct sFreeArea ** ppArea, SLONG size)
{
	struct sFreeArea *pArea;

	pArea = *ppArea;
	while (pArea) {
		if (size <= pArea->nSize) {
			SLONG r;

			r = pArea->nOrg;
			pArea->nOrg += size;
			pArea->nSize -= size;

			return (r);
		}
		ppArea = &(pArea->pNext);
		pArea = *ppArea;
	}

	return (-1);
}

SLONG 
area_AllocCODEAnyBank(SLONG size)
{
	SLONG i, org;

	for (i = 1; i <= 255; i += 1) {
		if ((org = area_Alloc(&BankFree[i], size)) != -1)
			return ((i << 16) | org);
	}

	return (-1);
}

struct sSection *
FindLargestCode(void)
{
	struct sSection *pSection, *r = NULL;
	SLONG nLargest = 0;

	pSection = pSections;
	while (pSection) {
		if (pSection->oAssigned == 0 && pSection->Type == SECT_CODE) {
			if (pSection->nByteSize > nLargest) {
				nLargest = pSection->nByteSize;
				r = pSection;
			}
		}
		pSection = pSection->pNext;
	}
	return (r);
}

void 
AssignCodeSections(void)
{
	struct sSection *pSection;

	while ((pSection = FindLargestCode())) {
		SLONG org;

		if ((org = area_AllocCODEAnyBank(pSection->nByteSize)) != -1) {
			pSection->nOrg = org & 0xFFFF;
			pSection->nBank = org >> 16;
			pSection->oAssigned = 1;
			DOMAXBANK(pSection->nBank);
		} else {
			fprintf(stderr,
			    "Unable to place CODE section anywhere\n");
			exit(1);
		}
	}
}

void 
AssignSections(void)
{
	SLONG i;
	struct sSection *pSection;

	MaxBankUsed = 0;

	/*
	 * Initialize the memory areas
	 *
	 */

	for (i = 0; i < MAXBANKS; i += 1) {
		BankFree[i] = malloc(sizeof *BankFree[i]);

		if (!BankFree[i]) {
			fprintf(stderr, "Out of memory!\n");
			exit(1);
		}

		if (i == 0) {
			BankFree[i]->nOrg = 0x0000;
			if (options & OPT_SMALL) {
				BankFree[i]->nSize = 0x8000;
				MaxAvail[i] = 0x8000;
			} else {
				BankFree[i]->nSize = 0x4000;
				MaxAvail[i] = 0x4000;
			}
		} else if (i >= 1 && i <= 255) {
			BankFree[i]->nOrg = 0x4000;
			/*
			 * Now, this shouldn't really be necessary... but for
			 * good measure we'll do it anyway.
			 */
			if (options & OPT_SMALL) {
				BankFree[i]->nSize = 0;
				MaxAvail[i] = 0;
			} else {
				BankFree[i]->nSize = 0x4000;
				MaxAvail[i] = 0x4000;
			}
		} else if (i == BANK_BSS) {
			BankFree[i]->nOrg = 0xC000;
			BankFree[i]->nSize = 0x2000;
			MaxAvail[i] = 0x2000;
		} else if (i == BANK_VRAM) {
			BankFree[i]->nOrg = 0x8000;
			BankFree[i]->nSize = 0x2000;
			MaxAvail[i] = 0x2000;
		} else if (i == BANK_HRAM) {
			BankFree[i]->nOrg = 0xFF80;
			BankFree[i]->nSize = 0x007F;
			MaxAvail[i] = 0x007F;
		}
		BankFree[i]->pPrev = NULL;
		BankFree[i]->pNext = NULL;
	}

	/*
	 * First, let's assign all the fixed sections...
	 * And all because of that Jens Restemeier character ;)
	 *
	 */

	pSection = pSections;
	while (pSection) {
		if ((pSection->nOrg != -1 || pSection->nBank != -1)
		    && pSection->oAssigned == 0) {
			/* User wants to have a say... */

			switch (pSection->Type) {
			case SECT_BSS:
				if (area_AllocAbs
				    (&BankFree[BANK_BSS], pSection->nOrg,
					pSection->nByteSize) != pSection->nOrg) {
					fprintf(stderr,
					    "Unable to load fixed BSS section "
					    "at $%lX\n", pSection->nOrg);
					exit(1);
				}
				pSection->oAssigned = 1;
				pSection->nBank = BANK_BSS;
				break;
			case SECT_HRAM:
				if (area_AllocAbs
				    (&BankFree[BANK_HRAM], pSection->nOrg,
					pSection->nByteSize) != pSection->nOrg) {
					fprintf(stderr, "Unable to load fixed "
					    "HRAM section at $%lX\n",
					    pSection->nOrg);
					exit(1);
				}
				pSection->oAssigned = 1;
				pSection->nBank = BANK_HRAM;
				break;
			case SECT_VRAM:
				if (area_AllocAbs
				    (&BankFree[BANK_VRAM], pSection->nOrg,
					pSection->nByteSize) != pSection->nOrg) {
					fprintf(stderr, "Unable to load fixed "
					    "VRAM section at $%lX\n",
					    pSection->nOrg);
					exit(1);
				}
				pSection->oAssigned = 1;
				pSection->nBank = BANK_VRAM;
				break;
			case SECT_HOME:
				if (area_AllocAbs
				    (&BankFree[BANK_HOME], pSection->nOrg,
					pSection->nByteSize) != pSection->nOrg) {
					fprintf(stderr, "Unable to load fixed "
					    "HOME section at $%lX\n",
					    pSection->nOrg);
					exit(1);
				}
				pSection->oAssigned = 1;
				pSection->nBank = BANK_HOME;
				break;
			case SECT_CODE:
				if (pSection->nBank == -1) {
					/*
					 * User doesn't care which bank, so he must want to
					 * decide which position within that bank.
					 * We'll do that at a later stage when the really
					 * hardcoded things are allocated
					 *
					 */
				} else {
					/*
					 * User wants to decide which bank we use
					 * Does he care about the position as well?
					 *
					 */

					if (pSection->nOrg == -1) {
						/*
						 * Nope, any position will do
						 * Again, we'll do that later
						 *
						 */
					} else {
						/*
						 * How hardcore can you possibly get? Why does
						 * he even USE this package? Yeah let's just
						 * direct address everything, shall we?
						 * Oh well, the customer is always right
						 *
						 */

						if (pSection->nBank >= 1
						    && pSection->nBank <= 255) {
							if (area_AllocAbs
							    (&BankFree
								[pSection->nBank],
								pSection->nOrg,
								pSection->
								nByteSize) !=
							    pSection->nOrg) {
								fprintf(stderr, "Unable to load fixed CODE/DATA section at $%lX in bank $%02lX\n", pSection->nOrg, pSection->nBank);
								exit(1);
							}
							DOMAXBANK(pSection->
							    nBank);
							pSection->oAssigned = 1;
						} else {
							fprintf(stderr, "Unable to load fixed CODE/DATA section at $%lX in bank $%02lX\n", pSection->nOrg, pSection->nBank);
							exit(1);
						}
					}

				}
				break;
			}
		}
		pSection = pSection->pNext;
	}

	/*
	 * Next, let's assign all the bankfixed ONLY CODE sections...
	 *
	 */

	pSection = pSections;
	while (pSection) {
		if (pSection->oAssigned == 0
		    && pSection->Type == SECT_CODE
		    && pSection->nOrg == -1 && pSection->nBank != -1) {
			/* User wants to have a say... and he's pissed */
			if (pSection->nBank >= 1 && pSection->nBank <= 255) {
				if ((pSection->nOrg =
					area_Alloc(&BankFree[pSection->nBank],
					    pSection->nByteSize)) == -1) {
					fprintf(stderr, "Unable to load fixed CODE/DATA section into bank $%02lX\n", pSection->nBank);
					exit(1);
				}
				pSection->oAssigned = 1;
				DOMAXBANK(pSection->nBank);
			} else {
				fprintf(stderr, "Unable to load fixed CODE/DATA section into bank $%02lX\n", pSection->nBank);
				exit(1);
			}
		}
		pSection = pSection->pNext;
	}

	/*
	 * Now, let's assign all the floating bank but fixed CODE sections...
	 *
	 */

	pSection = pSections;
	while (pSection) {
		if (pSection->oAssigned == 0
		    && pSection->Type == SECT_CODE
		    && pSection->nOrg != -1 && pSection->nBank == -1) {
			/* User wants to have a say... and he's back with a
			 * vengeance */
			if ((pSection->nBank =
				area_AllocAbsCODEAnyBank(pSection->nOrg,
				    pSection->nByteSize)) ==
			    -1) {
				fprintf(stderr, "Unable to load fixed CODE/DATA section at $%lX into any bank\n", pSection->nOrg);
				exit(1);
			}
			pSection->oAssigned = 1;
			DOMAXBANK(pSection->nBank);
		}
		pSection = pSection->pNext;
	}

	/*
	 * OK, all that nasty stuff is done so let's assign all the other
	 * sections
	 *
	 */

	pSection = pSections;
	while (pSection) {
		if (pSection->oAssigned == 0) {
			switch (pSection->Type) {
			case SECT_BSS:
				if ((pSection->nOrg =
					area_Alloc(&BankFree[BANK_BSS],
					    pSection->nByteSize)) == -1) {
					fprintf(stderr, "BSS section too large\n");
					exit(1);
				}
				pSection->nBank = BANK_BSS;
				pSection->oAssigned = 1;
				break;
			case SECT_HRAM:
				if ((pSection->nOrg =
					area_Alloc(&BankFree[BANK_HRAM],
					    pSection->nByteSize)) == -1) {
					fprintf(stderr, "HRAM section too large\n");
					exit(1);
				}
				pSection->nBank = BANK_HRAM;
				pSection->oAssigned = 1;
				break;
			case SECT_VRAM:
				if ((pSection->nOrg =
					area_Alloc(&BankFree[BANK_VRAM],
					    pSection->nByteSize)) == -1) {
					fprintf(stderr, "VRAM section too large\n");
					exit(1);
				}
				pSection->nBank = BANK_VRAM;
				pSection->oAssigned = 1;
				break;
			case SECT_HOME:
				if ((pSection->nOrg =
					area_Alloc(&BankFree[BANK_HOME],
					    pSection->nByteSize)) == -1) {
					fprintf(stderr, "HOME section too large\n");
					exit(1);
				}
				pSection->nBank = BANK_HOME;
				pSection->oAssigned = 1;
				break;
			case SECT_CODE:
				break;
			default:
				fprintf(stderr, "(INTERNAL) Unknown section type!\n");
				exit(1);
				break;
			}
		}
		pSection = pSection->pNext;
	}

	AssignCodeSections();
}

void 
CreateSymbolTable(void)
{
	struct sSection *pSect;

	sym_Init();

	pSect = pSections;

	while (pSect) {
		SLONG i;

		i = pSect->nNumberOfSymbols;

		while (i--) {
			if ((pSect->tSymbols[i]->Type == SYM_EXPORT) &&
			    ((pSect->tSymbols[i]->pSection == pSect) ||
				(pSect->tSymbols[i]->pSection == NULL))) {
				if (pSect->tSymbols[i]->pSection == NULL)
					sym_CreateSymbol(pSect->tSymbols[i]->
					    pzName,
					    pSect->tSymbols[i]->
					    nOffset, -1);
				else
					sym_CreateSymbol(pSect->tSymbols[i]->
					    pzName,
					    pSect->nOrg +
					    pSect->tSymbols[i]->
					    nOffset, pSect->nBank);
			}
		}
		pSect = pSect->pNext;
	}
}
