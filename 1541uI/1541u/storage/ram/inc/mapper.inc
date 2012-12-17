;-------------------------------------------------------------------------------
;--
;--  (C) COPYRIGHT 2007, Gideon's Hardware and Programmable Logic Solutions
;--
;-------------------------------------------------------------------------------
;-- Project    : Ultimate 1541
;-- Title      : mapper.inc
;-------------------------------------------------------------------------------
;-- Abstract:
;	Definitions allowing for readable use of the 65gz02 memory mapper.
;--
;-------------------------------------------------------------------------------

    MAPPER_MAP1L  = $2300
    MAPPER_MAP1H  = $2301
    MAPPER_MAP2L  = $2302
    MAPPER_MAP2H  = $2303
    MAPPER_MAP3L  = $2304
	MAPPER_MAP3H  = $2305
	MAPPER_CODE   = $2306
	
    MAP1_ADDR = $4000
    MAP2_ADDR = $6000
    MAP3_ADDR = $3000

	MAP_DATA = $0034
	MAP_SCR  = $0400
;    MAP_VIC  = $0406
    
	SCREEN1  = $4400 ; map1
	SCREEN2  = $6400 ; map2
;	COLOR1   = $5800 ; map1
;	COLOR2   = $7800 ; map2
    COLOR3   = $3800 ; map3
    	
    CIA1_DPA = $3C00 ; map3
    CIA1_DPB = $3C01 ; map3
    CIA1_ICR = $3C0D ; map3
    CIA2_ICR = $3D0D ; map3
    
    VIC_IREG = $3019 ; map3

    MAP_RAMDMA    = $0400 ;  800000
    MAP_FLASH     = $0800 ; 1000000
    MAP_SDRAM_CMD = $0C00 ; 1800000
    MAP_SDRAM     = $1000 ; 2000000
