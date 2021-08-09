
/*
  Copyright (C) 2000 Jesper Hansen <jesperh@telia.com>.

  This file is part of the yampp system.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "types.h"

void MAS_init(void);

void write_D0D1_mem(BYTE memtype, UINT address, UINT nWords, ULONG *data );
void write_register(BYTE reg, ULONG data);

void read_D0D1_mem(BYTE memtype, UINT address, UINT nWords, ULONG *data );
ULONG read_register(BYTE reg);

/*
USHORT default_read(void);
*/