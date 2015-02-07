

//---------------------------------------------------------------------------
// const defines
//---------------------------------------------------------------------------

#if defined (EPL_OBD_DEFINE_MACRO)

    //-------------------------------------------------------------------------------------------
#if defined (EPL_OBD_CREATE_ROM_DATA)

//        #pragma message ("EPL_OBD_CREATE_ROM_DATA")

#define EPL_OBD_BEGIN()                                                         static  DWORD  dwObd_OBK_g = 0x0000;
#define EPL_OBD_END()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_PART_GENERIC()
#define EPL_OBD_BEGIN_PART_MANUFACTURER()
#define EPL_OBD_BEGIN_PART_DEVICE()
#define EPL_OBD_END_PART()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_INDEX_RAM(ind,cnt,call)
#define EPL_OBD_END_INDEX(ind)
#define EPL_OBD_RAM_INDEX_RAM_ARRAY(ind,cnt,call,typ,acc,dtyp,name,def)         static  tEplObdUnsigned8    xDef##ind##_0x00_g = (cnt); \
                                                                                        static  dtyp  xDef##ind##_0x01_g = (def);
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY(ind,cnt,call,typ,acc,dtyp,name,def)      static  tEplObdUnsigned8    xDef##ind##_0x00_g = (cnt); \
                                                                                        static  dtyp  xDef##ind##_0x01_g = (def);
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY_NOINIT(ind,cnt,call,typ,acc,dtyp,name)   static  tEplObdUnsigned8    xDef##ind##_0x00_g = (cnt);

	//---------------------------------------------------------------------------------------
#define EPL_OBD_SUBINDEX_RAM_VAR(ind,sub,typ,acc,dtyp,name,val)                 static  dtyp  xDef##ind##_##sub##_g        = val;
#define EPL_OBD_SUBINDEX_RAM_VAR_RG(ind,sub,typ,acc,dtyp,name,val,low,high)     static  dtyp  xDef##ind##_##sub##_g[3]     = {val,low,high};
#define EPL_OBD_SUBINDEX_RAM_VAR_NOINIT(ind,sub,typ,acc,dtyp,name)
#define EPL_OBD_SUBINDEX_RAM_VSTRING(ind,sub,acc,name,size,val)                 static char  MEM szCur##ind##_##sub##_g[size+1]; \
                                                                                        static  tEplObdVStringDef  xDef##ind##_##sub##_g = {size, val, szCur##ind##_##sub##_g};

#define EPL_OBD_SUBINDEX_RAM_OSTRING(ind,sub,acc,name,size)                     static  BYTE  MEM bCur##ind##_##sub##_g[size]; \
                                                                                        static  tEplObdOStringDef  xDef##ind##_##sub##_g = {size, ((BYTE*)""), bCur##ind##_##sub##_g};
#define EPL_OBD_SUBINDEX_RAM_DOMAIN(ind,sub,acc,name)
#define EPL_OBD_SUBINDEX_RAM_USERDEF(ind,sub,typ,acc,dtyp,name,val)             static  dtyp  xDef##ind##_##sub##_g        = val;
#define EPL_OBD_SUBINDEX_RAM_USERDEF_RG(ind,sub,typ,acc,dtyp,name,val,low,high) static  dtyp  xDef##ind##_##sub##_g[3]     = {val,low,high};
#define EPL_OBD_SUBINDEX_RAM_USERDEF_NOINIT(ind,sub,typ,acc,dtyp,name)

//-------------------------------------------------------------------------------------------
#elif defined (EPL_OBD_CREATE_RAM_DATA)

//        #pragma message ("EPL_OBD_CREATE_RAM_DATA")

#define EPL_OBD_BEGIN()
#define EPL_OBD_END()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_PART_GENERIC()
#define EPL_OBD_BEGIN_PART_MANUFACTURER()
#define EPL_OBD_BEGIN_PART_DEVICE()
#define EPL_OBD_END_PART()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_INDEX_RAM(ind,cnt,call)
#define EPL_OBD_END_INDEX(ind)
#define EPL_OBD_RAM_INDEX_RAM_ARRAY(ind,cnt,call,typ,acc,dtyp,name,def)         static dtyp         MEM axCur##ind##_g[cnt];
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY(ind,cnt,call,typ,acc,dtyp,name,def)      static tEplObdVarEntry MEM aVarEntry##ind##_g[cnt];
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY_NOINIT(ind,cnt,call,typ,acc,dtyp,name)   static tEplObdVarEntry MEM aVarEntry##ind##_g[cnt];

	//---------------------------------------------------------------------------------------
#define EPL_OBD_SUBINDEX_RAM_VAR(ind,sub,typ,acc,dtyp,name,val)                 static dtyp         MEM xCur##ind##_##sub##_g;
#define EPL_OBD_SUBINDEX_RAM_VAR_RG(ind,sub,typ,acc,dtyp,name,val,low,high)     static dtyp         MEM xCur##ind##_##sub##_g;
#define EPL_OBD_SUBINDEX_RAM_VSTRING(ind,sub,acc,name,size,val)                 static tEplObdVString  MEM xCur##ind##_##sub##_g;
#define EPL_OBD_SUBINDEX_RAM_OSTRING(ind,sub,acc,name,size)                     static tEplObdOString  MEM xCur##ind##_##sub##_g;
#define EPL_OBD_SUBINDEX_RAM_VAR_NOINIT(ind,sub,typ,acc,dtyp,name)              static dtyp         MEM xCur##ind##_##sub##_g;
#define EPL_OBD_SUBINDEX_RAM_DOMAIN(ind,sub,acc,name)                           static tEplObdVarEntry MEM VarEntry##ind##_##sub##_g;
#define EPL_OBD_SUBINDEX_RAM_USERDEF(ind,sub,typ,acc,dtyp,name,val)             static tEplObdVarEntry MEM VarEntry##ind##_##sub##_g;
#define EPL_OBD_SUBINDEX_RAM_USERDEF_RG(ind,sub,typ,acc,dtyp,name,val,low,high) static tEplObdVarEntry MEM VarEntry##ind##_##sub##_g;
#define EPL_OBD_SUBINDEX_RAM_USERDEF_NOINIT(ind,sub,typ,acc,dtyp,name)          static tEplObdVarEntry MEM VarEntry##ind##_##sub##_g;

    //-------------------------------------------------------------------------------------------
#elif defined (EPL_OBD_CREATE_SUBINDEX_TAB)

//        #pragma message ("EPL_OBD_CREATE_SUBINDEX_TAB")

#define EPL_OBD_BEGIN()
#define EPL_OBD_END()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_PART_GENERIC()
#define EPL_OBD_BEGIN_PART_MANUFACTURER()
#define EPL_OBD_BEGIN_PART_DEVICE()
#define EPL_OBD_END_PART()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_INDEX_RAM(ind,cnt,call)                                   static tEplObdSubEntry MEM aObdSubEntry##ind##Ram_g[cnt]= {
#define EPL_OBD_END_INDEX(ind)                                                  EPL_OBD_END_SUBINDEX()};
#define EPL_OBD_RAM_INDEX_RAM_ARRAY(ind,cnt,call,typ,acc,dtyp,name,def)         static tEplObdSubEntry MEM aObdSubEntry##ind##Ram_g[]= { \
                                                                                        {0, kEplObdTypUInt8, kEplObdAccCR,          &xDef##ind##_0x00_g,   NULL}, \
                                                                                        {1, typ,          (acc)|kEplObdAccArray, &xDef##ind##_0x01_g,   &axCur##ind##_g[0]}, \
                                                                                        EPL_OBD_END_SUBINDEX()};
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY(ind,cnt,call,typ,acc,dtyp,name,def)      static tEplObdSubEntry MEM aObdSubEntry##ind##Ram_g[]= { \
                                                                                        {0, kEplObdTypUInt8, kEplObdAccCR,                     &xDef##ind##_0x00_g,   NULL}, \
                                                                                        {1, typ,          (acc)|kEplObdAccArray|kEplObdAccVar, &xDef##ind##_0x01_g,   &aVarEntry##ind##_g[0]}, \
                                                                                        EPL_OBD_END_SUBINDEX()};
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY_NOINIT(ind,cnt,call,typ,acc,dtyp,name)   static tEplObdSubEntry MEM aObdSubEntry##ind##Ram_g[]= { \
                                                                                        {0, kEplObdTypUInt8, kEplObdAccCR,                     &xDef##ind##_0x00_g,   NULL}, \
                                                                                        {1, typ,          (acc)|kEplObdAccArray|kEplObdAccVar, NULL,                  &aVarEntry##ind##_g[0]}, \
                                                                                        EPL_OBD_END_SUBINDEX()};

	//---------------------------------------------------------------------------------------
#define EPL_OBD_SUBINDEX_RAM_VAR(ind,sub,typ,acc,dtyp,name,val)                 {sub,typ,            (acc),                        &xDef##ind##_##sub##_g,   &xCur##ind##_##sub##_g},
#define EPL_OBD_SUBINDEX_RAM_VAR_RG(ind,sub,typ,acc,dtyp,name,val,low,high)     {sub,typ,            (acc)|kEplObdAccRange,           &xDef##ind##_##sub##_g[0],&xCur##ind##_##sub##_g},
#define EPL_OBD_SUBINDEX_RAM_VAR_NOINIT(ind,sub,typ,acc,dtyp,name)              {sub,typ,            (acc),                        NULL,   &xCur##ind##_##sub##_g},
#define EPL_OBD_SUBINDEX_RAM_VSTRING(ind,sub,acc,name,size,val)                 {sub,kEplObdTypVString,(acc)/*|kEplObdAccVar*/,         &xDef##ind##_##sub##_g,   &xCur##ind##_##sub##_g},
#define EPL_OBD_SUBINDEX_RAM_OSTRING(ind,sub,acc,name,size)                     {sub,kEplObdTypOString,(acc)/*|kEplObdAccVar*/,         &xDef##ind##_##sub##_g,   &xCur##ind##_##sub##_g},
#define EPL_OBD_SUBINDEX_RAM_DOMAIN(ind,sub,acc,name)                           {sub,kEplObdTypDomain, (acc)|kEplObdAccVar,             NULL,                     &VarEntry##ind##_##sub##_g},
#define EPL_OBD_SUBINDEX_RAM_USERDEF(ind,sub,typ,acc,dtyp,name,val)             {sub,typ,           (acc)|kEplObdAccVar,             &xDef##ind##_##sub##_g,   &VarEntry##ind##_##sub##_g},
#define EPL_OBD_SUBINDEX_RAM_USERDEF_RG(ind,sub,typ,acc,dtyp,name,val,low,high) {sub,typ,           (acc)|kEplObdAccVar|kEplObdAccRange,&xDef##ind##_##sub##_g[0],&VarEntry##ind##_##sub##_g},
#define EPL_OBD_SUBINDEX_RAM_USERDEF_NOINIT(ind,sub,typ,acc,dtyp,name)          {sub,typ,           (acc)|kEplObdAccVar,             NULL,    &VarEntry##ind##_##sub##_g},

    //-------------------------------------------------------------------------------------------
#elif defined (EPL_OBD_CREATE_INDEX_TAB)

//        #pragma message ("EPL_OBD_CREATE_INDEX_TAB")

#define EPL_OBD_BEGIN()
#define EPL_OBD_END()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_PART_GENERIC()                                                   static  tEplObdEntry  aObdTab_g[]      = {
#define EPL_OBD_BEGIN_PART_MANUFACTURER()                                       static  tEplObdEntry  aObdTabManufacturer_g[] = {
#define EPL_OBD_BEGIN_PART_DEVICE()                                             static  tEplObdEntry  aObdTabDevice_g[]       = {
#define EPL_OBD_END_PART()                                                      {EPL_OBD_TABLE_INDEX_END,(tEplObdSubEntryPtr)&dwObd_OBK_g,0,NULL}};

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_INDEX_RAM(ind,cnt,call)                                   {ind,(tEplObdSubEntryPtr)&aObdSubEntry##ind##Ram_g[0],cnt,(tEplObdCallback)call},
#define EPL_OBD_END_INDEX(ind)
#define EPL_OBD_RAM_INDEX_RAM_ARRAY(ind,cnt,call,typ,acc,dtyp,name,def)         {ind,(tEplObdSubEntryPtr)&aObdSubEntry##ind##Ram_g[0],(cnt)+1,(tEplObdCallback)call},
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY(ind,cnt,call,typ,acc,dtyp,name,def)      {ind,(tEplObdSubEntryPtr)&aObdSubEntry##ind##Ram_g[0],(cnt)+1,(tEplObdCallback)call},
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY_NOINIT(ind,cnt,call,typ,acc,dtyp,name)   {ind,(tEplObdSubEntryPtr)&aObdSubEntry##ind##Ram_g[0],(cnt)+1,(tEplObdCallback)call},

	//---------------------------------------------------------------------------------------
#define EPL_OBD_SUBINDEX_RAM_VAR(ind,sub,typ,acc,dtyp,name,val)
#define EPL_OBD_SUBINDEX_RAM_VAR_RG(ind,sub,typ,acc,dtyp,name,val,low,high)
#define EPL_OBD_SUBINDEX_RAM_VSTRING(ind,sub,acc,name,size,val)
#define EPL_OBD_SUBINDEX_RAM_OSTRING(ind,sub,acc,name,size)
#define EPL_OBD_SUBINDEX_RAM_VAR_NOINIT(ind,sub,typ,acc,dtyp,name)
#define EPL_OBD_SUBINDEX_RAM_DOMAIN(ind,sub,acc,name)
#define EPL_OBD_SUBINDEX_RAM_USERDEF(ind,sub,typ,acc,dtyp,name,val)
#define EPL_OBD_SUBINDEX_RAM_USERDEF_RG(ind,sub,typ,acc,dtyp,name,val,low,high)
#define EPL_OBD_SUBINDEX_RAM_USERDEF_NOINIT(ind,sub,typ,acc,dtyp,name)

	    //-------------------------------------------------------------------------------------------
#elif defined (EPL_OBD_CREATE_INIT_FUNCTION)

//        #pragma message ("EPL_OBD_CREATE_INIT_FUNCTION")

#define EPL_OBD_BEGIN()
#define EPL_OBD_END()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_PART_GENERIC()                                                   pInitParam->m_pPart      = (tEplObdEntryPtr) &aObdTab_g[0];
#define EPL_OBD_BEGIN_PART_MANUFACTURER()                                       pInitParam->m_pManufacturerPart = (tEplObdEntryPtr) &aObdTabManufacturer_g[0];
#define EPL_OBD_BEGIN_PART_DEVICE()                                             pInitParam->m_pDevicePart       = (tEplObdEntryPtr) &aObdTabDevice_g[0];
#define EPL_OBD_END_PART()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_INDEX_RAM(ind,cnt,call)
#define EPL_OBD_END_INDEX(ind)
#define EPL_OBD_RAM_INDEX_RAM_ARRAY(ind,cnt,call,typ,acc,dtyp,name,def)
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY(ind,cnt,call,typ,acc,dtyp,name,def)
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY_NOINIT(ind,cnt,call,typ,acc,dtyp,name)

	//---------------------------------------------------------------------------------------
#define EPL_OBD_SUBINDEX_RAM_VAR(ind,sub,typ,acc,dtyp,name,val)
#define EPL_OBD_SUBINDEX_RAM_VAR_RG(ind,sub,typ,acc,dtyp,name,val,low,high)
#define EPL_OBD_SUBINDEX_RAM_VSTRING(ind,sub,acc,name,size,val)
#define EPL_OBD_SUBINDEX_RAM_OSTRING(ind,sub,acc,name,size)
#define EPL_OBD_SUBINDEX_RAM_VAR_NOINIT(ind,sub,typ,acc,dtyp,name)
#define EPL_OBD_SUBINDEX_RAM_DOMAIN(ind,sub,acc,name)
#define EPL_OBD_SUBINDEX_RAM_USERDEF(ind,sub,typ,acc,dtyp,name,val)
#define EPL_OBD_SUBINDEX_RAM_USERDEF_RG(ind,sub,typ,acc,dtyp,name,val,low,high)
#define EPL_OBD_SUBINDEX_RAM_USERDEF_NOINIT(ind,sub,typ,acc,dtyp,name)

    //-------------------------------------------------------------------------------------------
#elif defined (EPL_OBD_CREATE_INIT_SUBINDEX)

//        #pragma message ("EPL_OBD_CREATE_INIT_SUBINDEX")

#define EPL_OBD_BEGIN()
#define EPL_OBD_END()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_PART_GENERIC()
#define EPL_OBD_BEGIN_PART_MANUFACTURER()
#define EPL_OBD_BEGIN_PART_DEVICE()
#define EPL_OBD_END_PART()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_INDEX_RAM(ind,cnt,call)	//CCM_SUBINDEX_RAM_ONLY (EPL_MEMCPY (&aObdSubEntry##ind##Ram_g[0],&aObdSubEntry##ind##Rom_g[0],sizeof(aObdSubEntry##ind##Ram_g)));
#define EPL_OBD_END_INDEX(ind)
#define EPL_OBD_RAM_INDEX_RAM_ARRAY(ind,cnt,call,typ,acc,dtyp,name,def)	//EPL_MEMCPY (&aObdSubEntry##ind##Ram_g[0],&aObdSubEntry##ind##Rom_g[0],sizeof(aObdSubEntry##ind##Ram_g));
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY(ind,cnt,call,typ,acc,dtyp,name,def)	//EPL_MEMCPY (&aObdSubEntry##ind##Ram_g[0],&aObdSubEntry##ind##Rom_g[0],sizeof(aObdSubEntry##ind##Ram_g));
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY_NOINIT(ind,cnt,call,typ,acc,dtyp,name)	//EPL_MEMCPY (&aObdSubEntry##ind##Ram_g[0],&aObdSubEntry##ind##Rom_g[0],sizeof(aObdSubEntry##ind##Ram_g));

	//---------------------------------------------------------------------------------------
#define EPL_OBD_SUBINDEX_RAM_VAR(ind,sub,typ,acc,dtyp,name,val)
#define EPL_OBD_SUBINDEX_RAM_VAR_RG(ind,sub,typ,acc,dtyp,name,val,low,high)
#define EPL_OBD_SUBINDEX_RAM_VSTRING(ind,sub,acc,name,size,val)
#define EPL_OBD_SUBINDEX_RAM_OSTRING(ind,sub,acc,name,size)
#define EPL_OBD_SUBINDEX_RAM_VAR_NOINIT(ind,sub,typ,acc,dtyp,name)
#define EPL_OBD_SUBINDEX_RAM_DOMAIN(ind,sub,acc,name)
#define EPL_OBD_SUBINDEX_RAM_USERDEF(ind,sub,typ,acc,dtyp,name,val)
#define EPL_OBD_SUBINDEX_RAM_USERDEF_RG(ind,sub,typ,acc,dtyp,name,val,low,high)
#define EPL_OBD_SUBINDEX_RAM_USERDEF_NOINIT(ind,sub,typ,acc,dtyp,name)

    //-------------------------------------------------------------------------------------------
#else

//        #pragma message ("ELSE OF DEFINE")

#define EPL_OBD_BEGIN()
#define EPL_OBD_END()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_PART_GENERIC()
#define EPL_OBD_BEGIN_PART_MANUFACTURER()
#define EPL_OBD_BEGIN_PART_DEVICE()
#define EPL_OBD_END_PART()

	//---------------------------------------------------------------------------------------
#define EPL_OBD_BEGIN_INDEX_RAM(ind,cnt,call)
#define EPL_OBD_END_INDEX(ind)
#define EPL_OBD_RAM_INDEX_RAM_ARRAY(ind,cnt,call,typ,acc,dtyp,name,def)
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY(ind,cnt,call,typ,acc,dtyp,name,def)
#define EPL_OBD_RAM_INDEX_RAM_VARARRAY_NOINIT(ind,cnt,call,typ,acc,dtyp,name)

	//---------------------------------------------------------------------------------------
#define EPL_OBD_SUBINDEX_RAM_VAR(ind,sub,typ,acc,dtyp,name,val)
#define EPL_OBD_SUBINDEX_RAM_VAR_RG(ind,sub,typ,acc,dtyp,name,val,low,high)
#define EPL_OBD_SUBINDEX_RAM_VSTRING(ind,sub,acc,name,sizes,val)
#define EPL_OBD_SUBINDEX_RAM_OSTRING(ind,sub,acc,name,size)
#define EPL_OBD_SUBINDEX_RAM_VAR_NOINIT(ind,sub,typ,acc,dtyp,name)
#define EPL_OBD_SUBINDEX_RAM_DOMAIN(ind,sub,acc,name)
#define EPL_OBD_SUBINDEX_RAM_USERDEF(ind,sub,typ,acc,dtyp,name,val)
#define EPL_OBD_SUBINDEX_RAM_USERDEF_RG(ind,sub,typ,acc,dtyp,name,val,low,high)
#define EPL_OBD_SUBINDEX_RAM_USERDEF_NOINIT(ind,sub,typ,acc,dtyp,name)

#endif

    //-------------------------------------------------------------------------------------------
#elif defined (EPL_OBD_UNDEFINE_MACRO)

//    #pragma message ("EPL_OBD_UNDEFINE_MACRO")

#undef EPL_OBD_BEGIN
#undef EPL_OBD_END

    //---------------------------------------------------------------------------------------
#undef EPL_OBD_BEGIN_PART_GENERIC
#undef EPL_OBD_BEGIN_PART_MANUFACTURER
#undef EPL_OBD_BEGIN_PART_DEVICE
#undef EPL_OBD_END_PART

    //---------------------------------------------------------------------------------------
#undef EPL_OBD_BEGIN_INDEX_RAM
#undef EPL_OBD_END_INDEX
#undef EPL_OBD_RAM_INDEX_RAM_ARRAY
#undef EPL_OBD_RAM_INDEX_RAM_VARARRAY
#undef EPL_OBD_RAM_INDEX_RAM_VARARRAY_NOINIT

    //---------------------------------------------------------------------------------------
#undef EPL_OBD_SUBINDEX_RAM_VAR
#undef EPL_OBD_SUBINDEX_RAM_VAR_RG
#undef EPL_OBD_SUBINDEX_RAM_VSTRING
#undef EPL_OBD_SUBINDEX_RAM_OSTRING
#undef EPL_OBD_SUBINDEX_RAM_VAR_NOINIT
#undef EPL_OBD_SUBINDEX_RAM_DOMAIN
#undef EPL_OBD_SUBINDEX_RAM_USERDEF
#undef EPL_OBD_SUBINDEX_RAM_USERDEF_RG
#undef EPL_OBD_SUBINDEX_RAM_USERDEF_NOINIT

#else

#error "nothing defined"

#endif
