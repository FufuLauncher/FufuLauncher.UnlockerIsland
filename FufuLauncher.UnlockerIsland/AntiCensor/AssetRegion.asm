; Copyright (c) FufuLauncher Dev Team. All rights reserved.
; Licensed under the AGPL-3.0 License.

EXTERN AntiCensorOriginalComparison:QWORD
EXTERN AntiCensorSelectBundle:QWORD
PUBLIC AntiCensorCompareRegion

.const
ALIGN 8

; Asset path hashes verified in the CN 7.2 streaming and persistent indexes.
CostumeAssets LABEL QWORD
    ; Qin
    dq 085B867189B3901A3h  ; UI_AvatarIcon_Qin
    dq 0DCF22DA5DE22EAD7h  ; UI_AvatarIcon_Qin_Card
    dq 08D6BD2C0C207A307h  ; UI_AvatarIcon_Qin_Circle
    dq 0C163CA3CF18DFC15h  ; Avatar_Lady_Sword_Qin
    dq 0982F6A1734D05A8Fh  ; Avatar_Lady_Sword_Qin_Edit
    dq 019EE2BC0F60F042Dh  ; Avatar_Lady_Sword_Qin_Kanban
    dq 014CB06A82D918EF0h  ; Avatar_Lady_Sword_Qin_Kanban_Edit
    dq 0E87B4AE7B0CB2961h  ; Avatar_Lady_Sword_Qin_Manekin
    dq 0A57CEA9CCECF2296h  ; Avatar_Lady_Sword_Qin_Remote
    dq 0AD3D81DEF51B4749h  ; NPC_Avatar_Lady_Sword_Qin
    dq 02BB46CB9359AB870h  ; NPC_Avatar_Lady_Sword_Qin_Edit
    dq 03287100986CF3885h  ; NPC_Homeworld_Avatar_Lady_Sword_Qin
    dq 043D78DE524805695h  ; NPC_Homeworld_Avatar_Lady_Sword_Qin_Edit
    dq 0F1DF6B82E3FFBED4h  ; Avatar_Lady_Sword_Qin_Model_Body
    dq 06E12CF44AB6C6FF5h  ; Avatar_Lady_Sword_Qin_Model_Face
    dq 024AB19F41F643586h  ; Avatar_Lady_Sword_Qin_Model_Face_Eye
    dq 0FB373680E8A32C66h  ; UI_AvatarIcon_Side_Qin
    dq 0143D9AC5A790BB26h  ; UI_Gacha_AvatarIcon_Qin
    dq 0B003C31BA64FC667h  ; UI_Gacha_AvatarImg_Qin
    ; Ambor
    dq 0BDA683F43F295895h  ; Avatar_Girl_Bow_Ambor
    dq 064FF87018ACA036Fh  ; Avatar_Girl_Bow_Ambor_Edit
    dq 0620EE6854A797D45h  ; Avatar_Girl_Bow_Ambor_Manekin
    dq 08F16A7BAF0603D6Fh  ; Avatar_Girl_Bow_Ambor_Model_Body
    dq 0C4AB550F4E93EF50h  ; Avatar_Girl_Bow_Ambor_Model_Face
    dq 03B56D36AB0F97734h  ; Avatar_Girl_Bow_Ambor_Model_Face_Eye
    dq 0097B9EEAF75B81D4h  ; UI_AvatarIcon_Side_Ambor
    dq 0E8C079120E3F24A4h  ; UI_Gacha_AvatarImg_Ambor
    dq 0E7BF47AF0121B5F6h  ; UI_AvatarIcon_Ambor
    dq 0158DB98E9B8F3DAFh  ; UI_AvatarIcon_Ambor_Card
    dq 02C1D687AE0AC0BE2h  ; UI_AvatarIcon_Ambor_Circle
    dq 0A56081AE3AE01AA8h  ; Avatar_Girl_Bow_Ambor_Kanban
    dq 0EED963793B179D84h  ; Avatar_Girl_Bow_Ambor_Kanban_Edit
    dq 084B9DFF5411BFDA5h  ; Avatar_Girl_Bow_Ambor_Remote
    dq 07E999FDC86B491EEh  ; NPC_Avatar_Girl_Bow_Ambor
    dq 0D5CEFFD774380F19h  ; NPC_Avatar_Girl_Bow_Ambor_Edit
    dq 036C14C5E79340339h  ; NPC_Homeworld_Avatar_Girl_Bow_Ambor
    dq 0DFACFC8C7E17B628h  ; NPC_Homeworld_Avatar_Girl_Bow_Ambor_Edit
    dq 0033A68B6D6B55ED5h  ; UI_Gacha_AvatarIcon_Ambor
    ; Mona
    dq 06B6B0AD28609B0E6h  ; UI_AvatarIcon_Mona
    dq 0635D334297D8A4F8h  ; UI_AvatarIcon_Mona_Card
    dq 0B3EB98E73CD00F63h  ; UI_AvatarIcon_Mona_Circle
    dq 0EBACB8FBE8F4A0D5h  ; Avatar_Girl_Catalyst_Mona
    dq 0DAADAA9D621B9CACh  ; Avatar_Girl_Catalyst_Mona_Edit
    dq 0925F39E4E799FEABh  ; Avatar_Girl_Catalyst_Mona_Kanban
    dq 0AD1F60C0BEBB383Fh  ; Avatar_Girl_Catalyst_Mona_Kanban_Edit
    dq 05637D70956527E49h  ; Avatar_Girl_Catalyst_Mona_Manekin
    dq 0DCF9573EA9A58FD7h  ; Avatar_Girl_Catalyst_Mona_Remote
    dq 0D5F630DDB3E67B91h  ; NPC_Avatar_Girl_Catalyst_Mona
    dq 0D0E5D223982333ADh  ; NPC_Avatar_Girl_Catalyst_Mona_Edit
    dq 0CEE758378ACE63D9h  ; NPC_Homeworld_Avatar_Girl_Catalyst_Mona
    dq 0C42095ADAE09DB3Eh  ; NPC_Homeworld_Avatar_Girl_Catalyst_Mona_Edit
    dq 0903D86FF283D25C8h  ; Avatar_Girl_Catalyst_Mona_Model_Body
    dq 00BA416E3D8474A42h  ; Avatar_Girl_Catalyst_Mona_Model_Face
    dq 0087343EBAB64B69Ah  ; Avatar_Girl_Catalyst_Mona_Model_Face_Eye
    dq 02F2ECE07DD9542C3h  ; UI_AvatarIcon_Side_Mona
    dq 00FFE4B2E75EDEB96h  ; UI_Gacha_AvatarIcon_Mona
    dq 070F061AC694BB293h  ; UI_Gacha_AvatarImg_Mona
    ; Rosaria
    dq 0238F6F7854BFBD69h  ; UI_AvatarIcon_Rosaria
    dq 05A6B0DC02B73CDF9h  ; UI_AvatarIcon_Rosaria_Card
    dq 0596FFF533342052Dh  ; UI_AvatarIcon_Rosaria_Circle
    dq 08A461F04442D24DDh  ; Avatar_Lady_Pole_Rosaria
    dq 0F2145EBB5D7DC73Eh  ; Avatar_Lady_Pole_Rosaria_Edit
    dq 02783633CA1723781h  ; Avatar_Lady_Pole_Rosaria_Kanban
    dq 0164C168C35EA2E6Dh  ; Avatar_Lady_Pole_Rosaria_Kanban_Edit
    dq 044F667266101C117h  ; Avatar_Lady_Pole_Rosaria_Manekin
    dq 0016B2219C554B5EBh  ; Avatar_Lady_Pole_Rosaria_Remote
    dq 041F28DF079507D61h  ; NPC_Avatar_Lady_Pole_Rosaria
    dq 0D1625B3345113A22h  ; NPC_Avatar_Lady_Pole_Rosaria_Edit
    dq 02CC1756FB5CB8D4Fh  ; NPC_Homeworld_Avatar_Lady_Pole_Rosaria
    dq 03EB8516DAC155D40h  ; NPC_Homeworld_Avatar_Lady_Pole_Rosaria_Edit
    dq 0DFB7215BE3221F4Ch  ; Avatar_Lady_Pole_Rosaria_Model_Body
    dq 03E41A73184DA2697h  ; Avatar_Lady_Pole_Rosaria_Model_Face
    dq 04B37069E66798BE1h  ; Avatar_Lady_Pole_Rosaria_Model_Face_Eye
    dq 027C51B5E4D28CAF5h  ; UI_AvatarIcon_Side_Rosaria
    dq 0A24924D875692BCCh  ; UI_Gacha_AvatarIcon_Rosaria
    dq 0EB6C6FA9293A2074h  ; UI_Gacha_AvatarImg_Rosaria
CostumeAssetsEnd LABEL QWORD

.code

AntiCensorCompareRegion PROC
    ; ReadAssetInfoMap_B owns the stack frame. R13 is the asset path hash,
    ; ECX is the region tag, EAX is the candidate and EDI is the current choice.
    ; R10 and R11 are dead at this site and through the CMOVE continuation.
    cmp edi, eax
    je nativeComparison
    cmp ecx, 1
    ja nativeComparison

    lea r10, CostumeAssets
    lea r11, CostumeAssetsEnd
compareAsset:
    cmp QWORD PTR [r10], r13
    je preferInternational
    add r10, 8
    cmp r10, r11
    jne compareAsset

nativeComparison:
    jmp QWORD PTR [AntiCensorOriginalComparison]

preferInternational:
    ; AssetRegion.OS is zero. The native CMOVE uses the ZF from this test.
    test ecx, ecx
    jmp QWORD PTR [AntiCensorSelectBundle]
AntiCensorCompareRegion ENDP

END
