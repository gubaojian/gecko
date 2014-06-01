/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsLayoutCID_h__
#define nsLayoutCID_h__

// {1691E1F4-EE41-11d4-9885-00C04FA0CF4B}
#define NS_FRAMETRAVERSAL_CID \
{ 0x1691e1f4, 0xee41, 0x11d4, { 0x98, 0x85, 0x0, 0xc0, 0x4f, 0xa0, 0xcf, 0x4b } }

/* a6cf90f9-15b3-11d2-932e-00805f8add32 */
#define NS_LAYOUT_DEBUGGER_CID \
 { 0xa6cf90f9, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

/* a6cf90d7-15b3-11d2-932e-00805f8add32 */
#define NS_FRAME_UTIL_CID \
 { 0xa6cf90d5, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

#define NS_DOMSELECTION_CID \
{/* {C87A37FC-8109-4ce2-A322-8CDEC925379F}*/ \
 0xc87a37fc, 0x8109, 0x4ce2, { 0xa3, 0x22, 0x8c, 0xde, 0xc9, 0x25, 0x37, 0x9f } }

// {D750A964-2D14-484c-B3AA-8ED7823B5C7B}
#define NS_BOXOBJECT_CID \
{ 0xd750a964, 0x2d14, 0x484c, { 0xb3, 0xaa, 0x8e, 0xd7, 0x82, 0x3b, 0x5c, 0x7b } }

// {C2710D40-6F4D-4b7f-9778-76AE5166648C}
#define NS_LISTBOXOBJECT_CID \
{ 0xc2710d40, 0x6f4d, 0x4b7f, { 0x97, 0x78, 0x76, 0xae, 0x51, 0x66, 0x64, 0x8c } }

// {56E2ADA8-4631-11d4-BA11-001083023C1E}
#define NS_SCROLLBOXOBJECT_CID \
{ 0x56e2ada8, 0x4631, 0x11d4, { 0xba, 0x11, 0x0, 0x10, 0x83, 0x2, 0x3c, 0x1e } }

// {AA40253B-4C42-4056-8132-37BCD07862FD}
#define NS_MENUBOXOBJECT_CID \
{ 0xaa40253b, 0x4c42, 0x4056, { 0x81, 0x32, 0x37, 0xbc, 0xd0, 0x78, 0x62, 0xfd } }

// {6C392C62-1AB1-4de7-BFC6-ED4F9FC7749A}
#define NS_POPUPBOXOBJECT_CID \
{ 0x6c392c62, 0x1ab1, 0x4de7, { 0xbf, 0xc6, 0xed, 0x4f, 0x9f, 0xc7, 0x74, 0x9a } }

// {dca9d7c3-7b6a-4490-ad6d-2699a8d683cd}
#define NS_CONTAINERBOXOBJECT_CID \
{ 0xdca9d7c3, 0x7b6a, 0x4490, { 0xad, 0x6d, 0x26, 0x99, 0xa8, 0xd6, 0x83, 0xcd } }

// {3B581FD4-3497-426c-8F61-3658B971CB80}
#define NS_TREEBOXOBJECT_CID \
{ 0x3b581fd4, 0x3497, 0x426c, { 0x8f, 0x61, 0x36, 0x58, 0xb9, 0x71, 0xcb, 0x80 } }

// {2fe88332-31c6-4829-b247-a07d8a73e80f}
#define NS_CANVASRENDERINGCONTEXTWEBGL_CID \
{ 0x2fe88332, 0x31c6, 0x4829, { 0xb2, 0x47, 0xa0, 0x7d, 0x8a, 0x7e, 0xe8, 0x0fe } }

// {A746DECD-AE74-4d86-8E75-4FDA81A9BE90}
#define NS_DOMSESSIONSTORAGEMANAGER_CID               \
{ 0xa746decd, 0xae74, 0x4d86, { 0x8e, 0x75, 0x4f, 0xda, 0x81, 0xa9, 0xbe, 0x90 } }

// {656DB07C-AA80-49e4-BCE8-E431BAAE697D}
#define NS_DOMLOCALSTORAGEMANAGER_CID               \
{ 0x656db07c, 0xaa80, 0x49e4, { 0xbc, 0xe8, 0xe4, 0x31, 0xba, 0xae, 0x69, 0x7d } }

// {93ad72a6-02cd-4716-9626-d47d5ec275ec}
#define NS_DOMJSON_CID \
{ 0x93ad72a6, 0x02cd, 0x4716, { 0x96, 0x26, 0xd4, 0x7d, 0x5e, 0xc2, 0x75, 0xec } }

// {CF7FD51F-ABA2-44C1-9FF0-11F7508EFCD4}
#define NS_FOCUSMANAGER_CID \
{ 0xcf7fd51f, 0xaba2, 0x44c1, { 0x9f, 0xf0, 0x11, 0xf7, 0x50, 0x8e, 0xfc, 0xd4 } }

// {1A26A7B7-D06E-4F45-8B45-D7AD60F7A9AB}
#define INDEXEDDB_MANAGER_CID \
{ 0x1a26a7b7, 0xd06e, 0x4f45, { 0x8b, 0x45, 0xd7, 0xad, 0x60, 0xf7, 0xa9, 0xab } }

// {3160e271-138d-4cc7-9d63-6429f16957c7}
#define DOMREQUEST_SERVICE_CID \
{ 0x3160e271, 0x138d, 0x4cc7, { 0x9d, 0x63, 0x64, 0x29, 0xf1, 0x69, 0x57, 0xc7 } }

// {5a75c25a-5e7e-4d90-8f7c-07eb15cc0aa8}
#define QUOTA_MANAGER_CID \
{ 0x5a75c25a, 0x5e7e, 0x4d90, { 0x8f, 0x7c, 0x07, 0xeb, 0x15, 0xcc, 0x0a, 0xa8 } }

// {c74bde32-bcc7-4840-8430-c733351b212a}
#define SERVICEWORKERMANAGER_CID \
{ 0xc74bde32, 0xbcc7, 0x4840, { 0x84, 0x30, 0xc7, 0x33, 0x35, 0x1b, 0x21, 0x2a } }

#endif /* nsLayoutCID_h__ */
