#include "UfsCommon.h"

VOID
SlsiUfsInitializeUpiuHeader(
    UFS_UPIU_HEADER *Header,
    UCHAR Type,
    UCHAR Lun,
    UCHAR TaskTag
)
{
    RtlZeroMemory(Header, sizeof(*Header));

    Header->Lun = Lun;
    Header->TaskTag = TaskTag;
}

