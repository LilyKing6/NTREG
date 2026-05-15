/*
 * PROJECT:   Registry Library
 * FILE:      security_descriptors.cpp
 * PURPOSE:   Pre-defined security descriptors for registry hives
 *
 * SECURITY_DESCRIPTOR structures extracted from registry hive security blocks ("sk").
 * Cross-checked against BCD, SOFTWARE, SYSTEM, SAM, and .DEFAULT system hives.
 */

#include "security_data.hpp"

unsigned char BcdSecurity[] =
{
    // SECU_DESC_RELATIVE
    0x01,                   // Revision
    0x00,                   // Sbz1
    0x04, 0x94,             // Control: SE_SELF_RELATIVE        (0x8000) |
                            //          SE_DACL_PROTECTED       (0x1000) |
                            //          SE_DACL_AUTO_INHERITED  (0x0400) |
                            //          SE_DACL_PRESENT         (0x0004)
    0x48, 0x00, 0x00, 0x00, // Owner
    0x58, 0x00, 0x00, 0x00, // Group
    0x00, 0x00, 0x00, 0x00, // Sacl (None)
    0x14, 0x00, 0x00, 0x00, // Dacl

    // DACL
    0x02,       // AclRevision
    0x00,       // Sbz1
    0x34, 0x00, // AclSize
    0x02, 0x00, // AceCount
    0x00, 0x00, // Sbz2

    // (1st ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x19, 0x00, 0x06, 0x00, // ACCESS_MASK: "Write DAC"         (0x00040000) |
                            //              "Read Control"      (0x00020000) |
                            //              "Notify"            (0x00000010) |
                            //              "Enumerate Subkeys" (0x00000008) |
                            //              "Query Value"       (0x00000001)
    // (SidStart: S-1-5-32-544 "Administrators")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00,

    // (2nd ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x14, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-5-18 "Local System")
    0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x12, 0x00, 0x00, 0x00,

    // Owner SID (S-1-5-32-544 "Administrators")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00,

    // Group SID (S-1-5-21-domain-513 "Domain Users")
    0x01, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00,
    0xAC, 0xD0, 0x49, 0xCB,
    0xE6, 0x52, 0x47, 0x9C,
    0xE4, 0x31, 0xDB, 0x5C,
    0x01, 0x02, 0x00, 0x00
};
const std::size_t BcdSecuritySize = sizeof(BcdSecurity);

unsigned char SoftwareSecurity[] =
{
    // SECU_DESC_RELATIVE
    0x01,                   // Revision
    0x00,                   // Sbz1
    0x04, 0x94,             // Control: SE_SELF_RELATIVE        (0x8000) |
                            //          SE_DACL_PROTECTED       (0x1000) |
                            //          SE_DACL_AUTO_INHERITED  (0x0400) |
                            //          SE_DACL_PRESENT         (0x0004)
    0xA0, 0x00, 0x00, 0x00, // Owner
    0xB0, 0x00, 0x00, 0x00, // Group
    0x00, 0x00, 0x00, 0x00, // Sacl (None)
    0x14, 0x00, 0x00, 0x00, // Dacl

    // DACL
    0x02,       // AclRevision
    0x00,       // Sbz1
    0x8C, 0x00, // AclSize
    0x06, 0x00, // AceCount
    0x00, 0x00, // Sbz2

    // (1st ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-5-32-544 "Administrators")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00,

    // (2nd ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x0A,                   // AceFlags: INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE
    0x14, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-3-0 "Creator Owner")
    0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x00,

    // (3rd ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x14, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-5-18 "Local System")
    0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x12, 0x00, 0x00, 0x00,

    // (4th ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x14, 0x00,             // AceSize
    0x1F, 0x00, 0x03, 0x00, // ACCESS_MASK: "Read Control"      (0x00020000) |
                            //              "Delete"            (0x00010000) |
                            //              "Notify"            (0x00000010) |
                            //              "Enumerate Subkeys" (0x00000008) |
                            //              "Create Subkey"     (0x00000004) |
                            //              "Set Value"         (0x00000002) |
                            //              "Query Value"       (0x00000001)
    // (SidStart: S-1-5-13 "Terminal Server Users")
    0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x0D, 0x00, 0x00, 0x00,

    // (5th ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x19, 0x00, 0x02, 0x00, // ACCESS_MASK: "Read Control"      (0x00020000) |
                            //              "Notify"            (0x00000010) |
                            //              "Enumerate Subkeys" (0x00000008) |
                            //              "Query Value"       (0x00000001)
    // (SidStart: S-1-5-32-545 "Users")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x21, 0x02, 0x00, 0x00,

    // (6th ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x1F, 0x00, 0x03, 0x00, // ACCESS_MASK: "Read Control"      (0x00020000) |
                            //              "Delete"            (0x00010000) |
                            //              "Notify"            (0x00000010) |
                            //              "Enumerate Subkeys" (0x00000008) |
                            //              "Create Subkey"     (0x00000004) |
                            //              "Set Value"         (0x00000002) |
                            //              "Query Value"       (0x00000001)
    // (SidStart: S-1-5-32-547 "Power Users")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x23, 0x02, 0x00, 0x00,

    // Owner SID (S-1-5-32-544 "Administrators")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00,

    // Group SID (S-1-5-21-domain-513 "Domain Users")
    0x01, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00,
    0xAC, 0xD0, 0x49, 0xCB,
    0xE6, 0x52, 0x47, 0x9C,
    0xE4, 0x31, 0xDB, 0x5C,
    0x01, 0x02, 0x00, 0x00
};
const std::size_t SoftwareSecuritySize = sizeof(SoftwareSecurity);

// Same security for SYSTEM, SAM and .DEFAULT
unsigned char SystemSecurity[] =
{
    // SECU_DESC_RELATIVE
    0x01,                   // Revision
    0x00,                   // Sbz1
    0x04, 0x94,             // Control: SE_SELF_RELATIVE        (0x8000) |
                            //          SE_DACL_PROTECTED       (0x1000) |
                            //          SE_DACL_AUTO_INHERITED  (0x0400) |
                            //          SE_DACL_PRESENT         (0x0004)
    0x8C, 0x00, 0x00, 0x00, // Owner
    0x9C, 0x00, 0x00, 0x00, // Group
    0x00, 0x00, 0x00, 0x00, // Sacl (None)
    0x14, 0x00, 0x00, 0x00, // Dacl

    // DACL
    0x02,       // AclRevision
    0x00,       // Sbz1
    0x78, 0x00, // AclSize
    0x05, 0x00, // AceCount
    0x00, 0x00, // Sbz2

    // (1st ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-5-32-544 "Administrators")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00,

    // (2nd ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x0A,                   // AceFlags: INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE
    0x14, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-3-0 "Creator Owner")
    0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x00,

    // (3rd ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x14, 0x00,             // AceSize
    0x3F, 0x00, 0x0F, 0x00, // ACCESS_MASK: "Full Control" (0x000F003F)
    // (SidStart: S-1-5-18 "Local System")
    0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x12, 0x00, 0x00, 0x00,

    // (4th ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x19, 0x00, 0x02, 0x00, // ACCESS_MASK: "Read Control"      (0x00020000) |
                            //              "Notify"            (0x00000010) |
                            //              "Enumerate Subkeys" (0x00000008) |
                            //              "Query Value"       (0x00000001)
    // (SidStart: S-1-5-32-545 "Users")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x21, 0x02, 0x00, 0x00,

    // (5th ACE)
    0x00,                   // AceType : ACCESS_ALLOWED_ACE_TYPE
    0x02,                   // AceFlags: CONTAINER_INHERIT_ACE
    0x18, 0x00,             // AceSize
    0x19, 0x00, 0x02, 0x00, // ACCESS_MASK: "Read Control"      (0x00020000) |
                            //              "Notify"            (0x00000010) |
                            //              "Enumerate Subkeys" (0x00000008) |
                            //              "Query Value"       (0x00000001)
    // (SidStart: S-1-5-32-547 "Power Users")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x23, 0x02, 0x00, 0x00,

    // Owner SID (S-1-5-32-544 "Administrators")
    0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x20, 0x00, 0x00, 0x00,
    0x20, 0x02, 0x00, 0x00,

    // Group SID (S-1-5-21-domain-513 "Domain Users")
    0x01, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x05,
    0x15, 0x00, 0x00, 0x00,
    0xAC, 0xD0, 0x49, 0xCB,
    0xE6, 0x52, 0x47, 0x9C,
    0xE4, 0x31, 0xDB, 0x5C,
    0x01, 0x02, 0x00, 0x00
};
const std::size_t SystemSecuritySize = sizeof(SystemSecurity);
