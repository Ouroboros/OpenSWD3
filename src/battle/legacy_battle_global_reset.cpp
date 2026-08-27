#include "openswd3/battle/legacy_battle_global_reset.hpp"

#include "openswd3/audio_video/legacy_sample_commands.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u32;

constexpr std::array<LegacyBattleGlobalResetWrite, 234> kResetWrites{{
    {0x00502940U, 4U, 1U, 0x00000000U},    // 0x0045B667
    {0x00502944U, 4U, 1U, 0x00000000U},    // 0x0045B671
    {0x00502948U, 4U, 1U, 0x00000000U},    // 0x0045B679
    {0x005240C4U, 2U, 1U, 0x00000000U},    // 0x0045B67E
    {0x0050294CU, 4U, 1U, 0x00000000U},    // 0x0045B685
    {0x00520DD0U, 4U, 10U, 0x00000000U},   // 0x0045B68A
    {0x00524200U, 4U, 8U, 0x00000000U},    // 0x0045B696
    {0x0053AF30U, 4U, 10U, 0x00000000U},   // 0x0045B6A2
    {0x00502950U, 4U, 1U, 0x00000000U},    // 0x0045B6A4
    {0x005244E8U, 4U, 126U, 0xFFFFFFFFU},  // 0x0045B6B5
    {0x005028ACU, 4U, 1U, 0x00000000U},    // 0x0045B6BB
    {0x005028B0U, 4U, 1U, 0x00000000U},    // 0x0045B6C6
    {0x005028B4U, 4U, 1U, 0x00000000U},    // 0x0045B6CC
    {0x005028B8U, 4U, 1U, 0x00000000U},    // 0x0045B6D2
    {0x005213A0U, 4U, 64U, 0x00000000U},   // 0x0045B6DD
    {0x0053AF70U, 4U, 80U, 0x00000000U},   // 0x0045B6E9
    {0x00524788U, 4U, 126U, 0x00000000U},  // 0x0045B6F5
    {0x0053AE70U, 4U, 7U, 0x00000000U},    // 0x0045B701
    {0x005202A8U, 4U, 684U, 0x00000000U},  // 0x0045B70D
    {0x00524980U, 4U, 684U, 0x00000000U},  // 0x0045B719
    {0x0053B0B8U, 4U, 731U, 0x00000000U},  // 0x0045B725
    {0x005028A0U, 4U, 1U, 0x00000000U},    // 0x0045B727
    {0x005028A4U, 4U, 1U, 0x00000000U},    // 0x0045B736
    {0x004FF1E0U, 4U, 67U, 0x00000000U},   // 0x0045B73B
    {0x00525470U, 4U, 38U, 0x00000000U},   // 0x0045B747
    {0x004FF168U, 4U, 20U, 0x00000000U},   // 0x0045B753
    {0x0052544CU, 4U, 1U, 0x00000000U},    // 0x0045B75C
    {0x00525450U, 4U, 1U, 0x00000000U},    // 0x0045B762
    {0x00525454U, 4U, 1U, 0x00000000U},    // 0x0045B768
    {0x00525458U, 4U, 1U, 0x00000000U},    // 0x0045B76E
    {0x00524324U, 4U, 60U, 0x00000000U},   // 0x0045B779
    {0x004FE5D4U, 4U, 10U, 0x00000000U},   // 0x0045B785
    {0x0052022CU, 4U, 20U, 0x00000000U},   // 0x0045B791
    {0x004FF0B0U, 4U, 1U, 0x00000000U},    // 0x0045B795
    {0x004FE5CCU, 4U, 1U, 0x00000000U},    // 0x0045B79A
    {0x004FF0B4U, 4U, 1U, 0x00000000U},    // 0x0045B7A0
    {0x004FE5D0U, 2U, 1U, 0x00000000U},    // 0x0045B7A5
    {0x004FF0B8U, 4U, 1U, 0x00000000U},    // 0x0045B7AC
    {0x005214F8U, 4U, 9U, 0x00000000U},    // 0x0045B7BB
    {0x004FF1B8U, 4U, 10U, 0x00000000U},   // 0x0045B7C7
    {0x0052027CU, 4U, 10U, 0x00000000U},   // 0x0045B7D3
    {0x00520F58U, 4U, 10U, 0x00000000U},   // 0x0045B7DF
    {0x0052100CU, 4U, 8U, 0x00000000U},    // 0x0045B7EB
    {0x004FF318U, 4U, 144U, 0x00000000U},  // 0x0045B7F7
    {0x0053AED8U, 4U, 1U, 0x00000000U},    // 0x0045B7F9
    {0x0053AEDCU, 4U, 1U, 0x00000000U},    // 0x0045B803
    {0x0053AEE0U, 4U, 1U, 0x00000000U},    // 0x0045B80D
    {0x004FF588U, 4U, 10U, 0x00000000U},   // 0x0045B817
    {0x00525470U, 4U, 38U, 0x00000000U},   // 0x0045B823
    {0x0052102CU, 4U, 10U, 0x00000000U},   // 0x0045B82F
    {0x005201D8U, 4U, 10U, 0x00000000U},   // 0x0045B83B
    {0x0053AEE4U, 4U, 1U, 0x00000000U},    // 0x0045B83F
    {0x004FF578U, 4U, 1U, 0x00000001U},    // 0x0045B844
    {0x004FF57CU, 4U, 1U, 0x00000001U},    // 0x0045B84F
    {0x004A7568U, 4U, 1U, 0x00000002U},    // 0x0045B855
    {0x004A7570U, 4U, 1U, 0x00000002U},    // 0x0045B85A
    {0x004FF580U, 4U, 1U, 0x00000001U},    // 0x0045B85F
    {0x004FF584U, 4U, 1U, 0x00000001U},    // 0x0045B86A
    {0x004CAE7CU, 4U, 1U, 0x00000000U},    // 0x0045B870
    {0x0053BC24U, 4U, 1U, 0x00000000U},    // 0x0045B876
    {0x0053BCDCU, 4U, 1U, 0x00000000U},    // 0x0045B87C
    {0x0053BCE0U, 4U, 1U, 0x00000000U},    // 0x0045B882
    {0x0053BCE4U, 4U, 1U, 0x00000000U},    // 0x0045B888
    {0x0053BF0CU, 2U, 1U, 0x00000000U},    // 0x0045B88E
    {0x0053BCE8U, 4U, 1U, 0x00000000U},    // 0x0045B895
    {0x0053BCECU, 4U, 1U, 0x00000000U},    // 0x0045B89B
    {0x004A7548U, 4U, 1U, 0x00000001U},    // 0x0045B8A1
    {0x004A754CU, 4U, 1U, 0x00000001U},    // 0x0045B8A7
    {0x0053BCF4U, 4U, 1U, 0x00000000U},    // 0x0045B8AD
    {0x004A7550U, 4U, 1U, 0x00000001U},    // 0x0045B8B3
    {0x004A7558U, 4U, 1U, 0x00000001U},    // 0x0045B8B9
    {0x004A755CU, 4U, 1U, 0x00000001U},    // 0x0045B8BF
    {0x004A756CU, 4U, 1U, 0x00000001U},    // 0x0045B8C5
    {0x004A7574U, 4U, 1U, 0xFFFFFFFFU},    // 0x0045B8CB
    {0x0053BD04U, 4U, 1U, 0x00000000U},    // 0x0045B8D1
    {0x0053BD08U, 4U, 1U, 0x00000000U},    // 0x0045B8D7
    {0x0053BD0CU, 4U, 1U, 0x00000000U},    // 0x0045B8DD
    {0x0053BD10U, 4U, 1U, 0x00000000U},    // 0x0045B8E3
    {0x0053BD14U, 4U, 1U, 0x00000000U},    // 0x0045B8E9
    {0x0053BD24U, 4U, 1U, 0x00000000U},    // 0x0045B8EF
    {0x0053BD28U, 4U, 1U, 0x00000000U},    // 0x0045B8F5
    {0x0053BD40U, 4U, 1U, 0x00000000U},    // 0x0045B8FB
    {0x004A7580U, 4U, 1U, 0x00000060U},    // 0x0045B901
    {0x0053BD4CU, 4U, 1U, 0x00000000U},    // 0x0045B906
    {0x0053BD50U, 4U, 1U, 0x00000000U},    // 0x0045B90C
    {0x0053BD54U, 4U, 1U, 0x00000000U},    // 0x0045B912
    {0x0053BD58U, 4U, 1U, 0x00000000U},    // 0x0045B918
    {0x004A7584U, 4U, 1U, 0x00000060U},    // 0x0045B91E
    {0x0053BD5CU, 4U, 1U, 0x00000000U},    // 0x0045B923
    {0x004A7588U, 4U, 1U, 0x000000F0U},    // 0x0045B929
    {0x0053BD60U, 4U, 1U, 0x00000000U},    // 0x0045B933
    {0x004A758CU, 4U, 1U, 0x000000FAU},    // 0x0045B939
    {0x004A7590U, 4U, 1U, 0x000000F2U},    // 0x0045B943
    {0x004A7594U, 4U, 1U, 0x00000064U},    // 0x0045B94D
    {0x0053BF46U, 2U, 1U, 0x00000000U},    // 0x0045B957
    {0x0053BF48U, 2U, 1U, 0x00000000U},    // 0x0045B95E
    {0x0053BD74U, 4U, 1U, 0x00000000U},    // 0x0045B965
    {0x00521374U, 4U, 1U, 0x00000000U},    // 0x0045B96F
    {0x004A7632U, 2U, 1U, 0x0000FFFFU},    // 0x0045B974
    {0x004A7634U, 2U, 1U, 0x0000FFFFU},    // 0x0045B97B
    {0x004A7636U, 2U, 1U, 0x0000FFFFU},    // 0x0045B982
    {0x004A7630U, 2U, 1U, 0x0000FFFFU},    // 0x0045B989
    {0x004A7564U, 4U, 1U, 0xFFFFFFFFU},    // 0x0045B990
    {0x004A762CU, 2U, 1U, 0x0000FFFFU},    // 0x0045B996
    {0x004A7644U, 2U, 1U, 0x0000FFFFU},    // 0x0045B99D
    {0x00521378U, 4U, 1U, 0x00000000U},    // 0x0045B9A6
    {0x004FF2F0U, 4U, 1U, 0x00000000U},    // 0x0045B9AB
    {0x0052137CU, 4U, 1U, 0x00000000U},    // 0x0045B9B1
    {0x004FF2F4U, 4U, 1U, 0x00000000U},    // 0x0045B9B6
    {0x00521380U, 4U, 1U, 0x00000000U},    // 0x0045B9BC
    {0x00520FBCU, 4U, 1U, 0x00000000U},    // 0x0045B9C1
    {0x004FF2F8U, 4U, 1U, 0x00000000U},    // 0x0045B9C7
    {0x00521384U, 2U, 1U, 0x00000000U},    // 0x0045B9CD
    {0x00520FC0U, 4U, 1U, 0x00000000U},    // 0x0045B9D3
    {0x004FF2FCU, 4U, 1U, 0x00000000U},    // 0x0045B9DE
    {0x0053BD78U, 4U, 1U, 0x00000000U},    // 0x0045B9E4
    {0x0053BD7CU, 4U, 1U, 0x00000000U},    // 0x0045B9EA
    {0x0053BD80U, 4U, 1U, 0x00000000U},    // 0x0045B9F0
    {0x0053BD84U, 4U, 1U, 0x00000000U},    // 0x0045B9F6
    {0x0053BD88U, 4U, 1U, 0x00000000U},    // 0x0045B9FC
    {0x0053BD9CU, 4U, 1U, 0x00000000U},    // 0x0045BA02
    {0x0053BDA0U, 4U, 1U, 0x00000000U},    // 0x0045BA08
    {0x0053BF12U, 2U, 1U, 0x00000000U},    // 0x0045BA0E
    {0x0053BF14U, 2U, 1U, 0x00000000U},    // 0x0045BA15
    {0x0053BF16U, 2U, 1U, 0x00000000U},    // 0x0045BA1C
    {0x0053BF1CU, 2U, 1U, 0x00000000U},    // 0x0045BA23
    {0x0053BDACU, 4U, 1U, 0x00000000U},    // 0x0045BA2A
    {0x0053BF1EU, 2U, 1U, 0x00000000U},    // 0x0045BA30
    {0x0053BF20U, 2U, 1U, 0x00000000U},    // 0x0045BA37
    {0x004A75FEU, 1U, 1U, 0x00000010U},    // 0x0045BA3E
    {0x00520FC4U, 2U, 1U, 0x00000000U},    // 0x0045BA45
    {0x0053BEFFU, 1U, 1U, 0x00000000U},    // 0x0045BA4C
    {0x0053BF00U, 1U, 1U, 0x00000000U},    // 0x0045BA52
    {0x0053BF02U, 1U, 1U, 0x00000000U},    // 0x0045BA58
    {0x0053BF04U, 4U, 1U, 0x00000000U},    // 0x0045BA5E
    {0x0053BF08U, 4U, 1U, 0x00000000U},    // 0x0045BA64
    {0x004A7620U, 2U, 1U, 0x0000FFFFU},    // 0x0045BA6A
    {0x004A7622U, 2U, 1U, 0x0000FFFFU},    // 0x0045BA70
    {0x004A7624U, 2U, 1U, 0x0000FFFFU},    // 0x0045BA76
    {0x004A7626U, 2U, 1U, 0x0000FFFFU},    // 0x0045BA7C
    {0x004FDF7CU, 2U, 1U, 0x00000000U},    // 0x0045BA82
    {0x005242F8U, 2U, 1U, 0x00000000U},    // 0x0045BA89
    {0x004A7628U, 2U, 1U, 0x00000006U},    // 0x0045BA90
    {0x0053BF0EU, 2U, 1U, 0x00000000U},    // 0x0045BA99
    {0x0052441CU, 4U, 1U, 0x00000000U},    // 0x0045BAA0
    {0x0053AE8CU, 4U, 1U, 0x00000000U},    // 0x0045BAA6
    {0x0053BF36U, 2U, 1U, 0x00000000U},    // 0x0045BAAC
    {0x0053BF38U, 2U, 1U, 0x00000000U},    // 0x0045BAB3
    {0x0053BF3AU, 2U, 1U, 0x00000000U},    // 0x0045BABA
    {0x00521520U, 2U, 1U, 0x00000000U},    // 0x0045BAC1
    {0x0053BD90U, 4U, 1U, 0x00000000U},    // 0x0045BAC8
    {0x0053BD94U, 4U, 1U, 0x00000000U},    // 0x0045BACE
    {0x0053BD98U, 4U, 1U, 0x00000000U},    // 0x0045BAD4
    {0x0053BF22U, 2U, 1U, 0x00000000U},    // 0x0045BADA
    {0x0053BF24U, 2U, 1U, 0x00000000U},    // 0x0045BAE1
    {0x0053BF26U, 2U, 1U, 0x00000000U},    // 0x0045BAE8
    {0x0053BD20U, 4U, 1U, 0x00000000U},    // 0x0045BAEF
    {0x0053BDEEU, 1U, 1U, 0x00000000U},    // 0x0045BAF5
    {0x004FDFA4U, 4U, 1U, 0x00000000U},    // 0x0045BAFB
    {0x00521394U, 4U, 1U, 0x00000000U},    // 0x0045BB01
    {0x00525468U, 4U, 1U, 0x00000000U},    // 0x0045BB07
    {0x004FDF8CU, 4U, 1U, 0x00000000U},    // 0x0045BB0D
    {0x00521388U, 4U, 1U, 0x00000000U},    // 0x0045BB13
    {0x00525448U, 4U, 1U, 0x00000000U},    // 0x0045BB19
    {0x0052151CU, 4U, 1U, 0x00000000U},    // 0x0045BB1F
    {0x00525430U, 4U, 1U, 0x00000000U},    // 0x0045BB25
    {0x00520D58U, 4U, 1U, 0x00000000U},    // 0x0045BB2B
    {0x00520FB8U, 4U, 1U, 0x00000000U},    // 0x0045BB31
    {0x0053BF28U, 2U, 1U, 0x00000000U},    // 0x0045BB37
    {0x0053BF2AU, 2U, 1U, 0x00000000U},    // 0x0045BB3E
    {0x0053BD68U, 4U, 1U, 0x00000000U},    // 0x0045BB45
    {0x0053C4C0U, 1U, 1U, 0x00000000U},    // 0x0045BB4B
    {0x0053C498U, 2U, 1U, 0x00000000U},    // 0x0045BB51
    {0x004FF300U, 4U, 1U, 0x00000000U},    // 0x0045BB58
    {0x00525434U, 4U, 1U, 0x00000000U},    // 0x0045BB65
    {0x00525438U, 4U, 1U, 0x00000000U},    // 0x0045BB6F
    {0x0053BF5CU, 4U, 1U, 0x00000000U},    // 0x0045BB74
    {0x0052543CU, 4U, 1U, 0x00000000U},    // 0x0045BB7A
    {0x0053BF60U, 4U, 1U, 0x00000000U},    // 0x0045BB7F
    {0x00525440U, 4U, 1U, 0x00000000U},    // 0x0045BB85
    {0x0053BF64U, 4U, 1U, 0x00000000U},    // 0x0045BB8A
    {0x00524468U, 4U, 10U, 0x00000000U},   // 0x0045BB90
    {0x00520E40U, 4U, 18U, 0x00000000U},   // 0x0045BB9C
    {0x005214ACU, 4U, 18U, 0x00000000U},   // 0x0045BBA8
    {0x00520D5CU, 4U, 18U, 0x00000000U},   // 0x0045BBB4
    {0x00524268U, 4U, 8U, 0x00000000U},    // 0x0045BBC0
    {0x0052411CU, 4U, 18U, 0x00000000U},   // 0x0045BBCC
    {0x00525444U, 4U, 1U, 0x00000000U},    // 0x0045BBCE
    {0x0053BF68U, 4U, 1U, 0x00000000U},    // 0x0045BBD3
    {0x0053BF6CU, 4U, 1U, 0x00000000U},    // 0x0045BBD9
    {0x0053BF70U, 4U, 1U, 0x00000000U},    // 0x0045BBDF
    {0x0053BF74U, 4U, 1U, 0x00000000U},    // 0x0045BBE5
    {0x0053BF78U, 4U, 1U, 0x00000000U},    // 0x0045BBEB
    {0x0053BF7CU, 4U, 1U, 0x00000000U},    // 0x0045BBF1
    {0x0053BF80U, 4U, 1U, 0x00000000U},    // 0x0045BBF7
    {0x0053BF88U, 4U, 1U, 0x00000000U},    // 0x0045BBFD
    {0x0053BF8CU, 4U, 1U, 0x00000000U},    // 0x0045BC03
    {0x0053BF90U, 4U, 1U, 0x00000000U},    // 0x0045BC09
    {0x0053BF94U, 4U, 1U, 0x00000000U},    // 0x0045BC0F
    {0x0053BF98U, 4U, 1U, 0x00000000U},    // 0x0045BC15
    {0x0053BF9CU, 4U, 1U, 0x00000000U},    // 0x0045BC1B
    {0x0053BFA0U, 4U, 1U, 0x00000000U},    // 0x0045BC21
    {0x0053BFA4U, 4U, 1U, 0x00000000U},    // 0x0045BC27
    {0x0053BFA8U, 4U, 1U, 0x00000000U},    // 0x0045BC2D
    {0x0053BFACU, 4U, 1U, 0x00000000U},    // 0x0045BC33
    {0x0053BFB0U, 4U, 1U, 0x00000000U},    // 0x0045BC39
    {0x0053BFB4U, 4U, 1U, 0x00000000U},    // 0x0045BC3F
    {0x0053BFB8U, 4U, 1U, 0x00000000U},    // 0x0045BC45
    {0x0053BFBCU, 4U, 1U, 0x00000000U},    // 0x0045BC4B
    {0x0053BFC0U, 4U, 1U, 0x00000000U},    // 0x0045BC51
    {0x0053BFC4U, 4U, 1U, 0x00000000U},    // 0x0045BC57
    {0x0053BFCCU, 4U, 1U, 0x00000000U},    // 0x0045BC5D
    {0x0053BFD0U, 4U, 1U, 0x00000000U},    // 0x0045BC63
    {0x0053BFD4U, 4U, 1U, 0x00000000U},    // 0x0045BC69
    {0x0053BFE0U, 4U, 1U, 0x00000000U},    // 0x0045BC6F
    {0x0053BFD8U, 4U, 1U, 0x00000000U},    // 0x0045BC75
    {0x0053BFDCU, 4U, 1U, 0x00000000U},    // 0x0045BC7B
    {0x0053BFE4U, 4U, 1U, 0x00000000U},    // 0x0045BC81
    {0x0053BFF0U, 4U, 1U, 0x00000000U},    // 0x0045BC87
    {0x0053BFF4U, 4U, 1U, 0x00000000U},    // 0x0045BC8D
    {0x004A763CU, 4U, 1U, 0x00000001U},    // 0x0045BC93
    {0x0053BFFCU, 4U, 1U, 0x00000000U},    // 0x0045BC99
    {0x0053C000U, 4U, 1U, 0x00000000U},    // 0x0045BC9F
    {0x0053C050U, 2U, 1U, 0x00000000U},    // 0x0045BCA5
    {0x0053C004U, 4U, 1U, 0x00000000U},    // 0x0045BCAC
    {0x0053C018U, 4U, 1U, 0x00000000U},    // 0x0045BCB2
    {0x0053C028U, 4U, 1U, 0x00000000U},    // 0x0045BCB8
    {0x0053C4A0U, 4U, 1U, 0x00000000U},    // 0x0045BCBE
    {0x0053CEACU, 4U, 1U, 0x00000000U},    // 0x0045BCC4
    {0x0053C038U, 4U, 1U, 0x00000000U},    // 0x0045BCCA
    {0x0053C040U, 4U, 1U, 0x00000000U},    // 0x0045BCD0
    {0x0053C048U, 4U, 1U, 0x00000000U},    // 0x0045BCD6
    {0x0053BFFCU, 4U, 1U, 0x00000000U},    // 0x0045BD01
    {0x0053C154U, 4U, 6U, 0x00000000U},    // 0x0045BD07
}};

struct MappedRange {
    u32 address;
    u32 bytes;
};

constexpr std::array<MappedRange, 76> kMappedRanges{{
    {0x004A754CU, 0x04U},  {0x004A7564U, 0x04U},  {0x004A7620U, 0x08U},
    {0x004A7630U, 0x02U},  {0x004A7644U, 0x02U},  {0x004FDF7CU, 0x02U},
    {0x004FDF8CU, 0x04U},  {0x004FDFA4U, 0x04U},  {0x004FE5CCU, 0x2CU},
    {0x00520D58U, 0x04U},  {0x00520FB8U, 0x04U},  {0x00521388U, 0x04U},
    {0x00521394U, 0x04U},  {0x0052151CU, 0x04U},  {0x00525430U, 0x04U},
    {0x00525448U, 0x04U},  {0x00525468U, 0x04U},  {0x004FF0B0U, 0x0CU},
    {0x004FF168U, 0x50U},  {0x0053AF30U, 0x28U},  {0x00502940U, 0x14U},
    {0x0052022CU, 0x50U},  {0x005202A8U, 0xAB0U}, {0x00520DD0U, 0x28U},
    {0x00520D5CU, 0x48U},  {0x00520E40U, 0x48U},  {0x005213A0U, 0x100U},
    {0x005214ACU, 0x48U},  {0x005214F8U, 0x24U},  {0x0052411CU, 0x48U},
    {0x00524268U, 0x20U},  {0x00524324U, 0xF0U},  {0x005244E8U, 0x1F8U},
    {0x00524788U, 0x1F8U}, {0x0052544CU, 0x10U},  {0x00525470U, 0x98U},
    {0x0053AE70U, 0x1CU},  {0x0053AF70U, 0x140U}, {0x0053B0B8U, 0xB6CU},
    {0x0053BCE0U, 0x04U},  {0x0053BCE8U, 0x04U},  {0x0053BC24U, 0x04U},
    {0x0052441CU, 0x04U},  {0x0053BCF4U, 0x04U},  {0x0053BD40U, 0x04U},
    {0x0053BD50U, 0x08U},  {0x0053BD5CU, 0x04U},  {0x0053BD7CU, 0x10U},
    {0x0053BDA0U, 0x04U},  {0x0053BCECU, 0x04U},  {0x0053BEFFU, 0x01U},
    {0x0053BF00U, 0x01U},  {0x0053BF0CU, 0x02U},  {0x0053BF24U, 0x02U},
    {0x0053BF5CU, 0x04U},  {0x0053BF60U, 0x04U},  {0x0053BF64U, 0x08U},
    {0x0053BF74U, 0x04U},  {0x0053BF7CU, 0x04U},  {0x0053BF80U, 0x04U},
    {0x0053BFA8U, 0x04U},  {0x0053BFC0U, 0x08U},  {0x0053BFB8U, 0x08U},
    {0x0053BFD0U, 0x04U},  {0x0053BFD8U, 0x04U},  {0x0053BFCCU, 0x04U},
    {0x0053BFF4U, 0x04U},  {0x0053C000U, 0x04U},  {0x0053C018U, 0x04U},
    {0x0053BFE4U, 0x04U},  {0x0053C040U, 0x04U},  {0x0053C048U, 0x04U},
    {0x0053C050U, 0x02U},  {0x0053C4A0U, 0x04U},  {0x0053C4C0U, 0x01U},
    {0x0053CEACU, 0x04U},
}};

[[nodiscard]] bool is_mapped_byte(const u32 address) noexcept {
    for (const auto& range : kMappedRanges) {
        if (address >= range.address && address - range.address < range.bytes) {
            return true;
        }
    }
    return false;
}

void apply_write(
    LegacyBattleGlobalResetState& state,
    const LegacyBattleGlobalResetWrite& write,
    LegacyBattleGlobalResetResult& result
) {
    state.write_trace.push_back(write);
    ++result.write_operations;
    result.physical_writes += write.count;
    result.bytes_written += write.size * write.count;
    for (u32 index = 0U; index < write.count; ++index) {
        const u32 base = write.address + index * write.size;
        for (u32 byte = 0U; byte < write.size; ++byte) {
            const u32 address = base + byte;
            if (is_mapped_byte(address)) {
                state.unmapped_bytes.erase(address);
            } else {
                state.unmapped_bytes[address] =
                    static_cast<u8>(write.value >> (byte * 8U));
            }
        }
    }
}

template <typename T> void clear_records(T& records) noexcept {
    for (auto& record : records) {
        record = {};
    }
}

void synchronize_typed_aliases(
    LegacyBattleStartupState& startup,
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleActionDispatchState& action,
    LegacyBattleGroupBFrameState& actor_frames,
    u32& message_state,
    u32& terminal_latch,
    u32& pair_primary_value,
    rendering::LegacyFrameColorTransitionState& color_accumulation,
    LegacyBattleActorMetricState& metrics,
    LegacyBattleEffectShiftState& shift,
    LegacyBattleEffectCoordinatorState& coordinator,
    LegacyBattleDebugHotkeyState& debug_hotkeys,
    LegacyBattleDebugOverlayState& debug_overlay,
    u32& debug_overlay_gate,
    LegacyBattleRetreatCommitState& retreat_commit,
    LegacyBattleOutcomeResolutionState& outcome_resolution,
    LegacyBattleOutcomeFinalizationState& outcome_finalization,
    LegacyBattleVerticalShiftState& vertical_shift
) {
    startup.render_geometry = {};
    auto& reset = startup.reset;
    reset.block_525470.fill(0U);
    reset.block_4ff168.fill(0U);
    reset.block_524324.fill(0U);
    reset.block_4fe5d4.fill(0U);
    reset.block_52022c.fill(0U);
    reset.block_5214f8.fill(0U);
    reset.block_524268.fill(0U);
    reset.block_5244e8.fill(0xFFFFFFFFU);
    reset.values_52544c.fill(0U);
    reset.values_502940.fill(0U);
    reset.value_4ff0b0 = 0U;
    reset.value_4fe5cc = 0U;
    reset.value_4ff0b4 = 0U;
    reset.value_4fe5d0 = 0U;
    reset.value_4ff0b8 = 0U;
    reset.value_53bf22 = 0U;
    reset.value_53bf80 = 0U;
    reset.value_53bfd0 = 0U;
    reset.value_53c048 = 0U;
    for (auto& record : reset.records_524788) {
        record = {};
        record.value_00 = 0U;
    }
    clear_records(startup.enemies);
    clear_records(startup.party);
    startup.enemy_count = 0U;
    startup.party_count = 0U;
    startup.mirror_mode = 0U;

    final_actor.active_actor_code = 0U;
    final_actor.secondary_actor_code = 0U;
    final_actor.published_actor_code = 1U;
    final_actor.source_actor_code = 0xFFFFFFFFU;
    final_actor.action_execution_active = 0U;
    final_actor.auxiliary_gate = 0U;
    final_actor.pre_frame_gate_a = 0U;
    final_actor.pre_frame_gate_b = 0U;
    final_actor.frame_gate_a = 0U;
    final_actor.frame_gate_b = 0U;
    final_actor.selection_gate = 0U;
    final_actor.queued_actor_code = 0U;
    final_actor.actor_order.fill(0U);
    final_actor.removed_group_a_count = 0U;
    final_actor.excluded_group_a_count = 0U;
    message_state = 0U;
    terminal_latch = 0U;
    pair_primary_value = 0U;
    action.phase_counter &= 0xFFFF0000U;
    action.packed_actor_counter &= 0xFFFFFF00U;
    action.message_gate = 0U;
    std::fill_n(action.opponent_workspace.begin(), 10U, 0U);
    std::fill_n(action.opponent_workspace.begin() + 16U, 80U, 0U);

    actor_frames.shared.selection_aux_gate = 0U;
    actor_frames.shared.target_ready_gate = 0U;
    actor_frames.shared.action_block_gate = 0U;
    actor_frames.shared.action.action_pending_aux = 0U;

    color_accumulation = {};

    metrics.values.fill(0);
    metrics.actor_order.fill(0U);
    metrics.selected_mask.fill(0U);
    metrics.group_b_count = 0U;
    metrics.group_a_count = 0U;
    metrics.priority_update_gate = 0U;
    metrics.group_a_mode = 0U;
    metrics.group_b_mode = 0U;
    metrics.priority_actor_index = 0U;
    metrics.priority_order_ready = 0U;
    metrics.pending_action_activation_latch = 0U;

    shift.actor_delta = 0;
    shift.direction_mode = 0U;
    shift.threshold_word = 0U;
    shift.completion_latch = 0U;

    coordinator.primary.fill({});
    coordinator.intensity_records.fill({});
    coordinator.required_completion_count = 0U;
    coordinator.group_a_global_gate = 0U;
    coordinator.group_a_effect_mode = 0U;
    coordinator.group_b_effect_mode = 0U;
    coordinator.group_b_argument = 0U;
    coordinator.completion_target_count = 0U;
    coordinator.completed_count = 0U;
    coordinator.group_b_render_count = 0U;
    coordinator.group_a_render_count = 0U;
    coordinator.focus_release_latch = 0U;
    coordinator.actor_activity_latch = 0U;
    coordinator.group_activity_latch = 0U;
    coordinator.framebuffer_dirty_latch = 0U;
    coordinator.queried_actor_word = 0U;
    coordinator.selected_actor_pair =
        (coordinator.selected_actor_pair & 0xFFFF0000U) | 0x0000FFFFU;
    coordinator.group_a_feedback_actor = 0xFFFFU;
    coordinator.group_b_feedback_actor = 0xFFFFU;
    coordinator.group_a_arguments.fill(0U);

    debug_hotkeys.selection_status_word_53c050 &= 0xFFFF0000U;
    debug_hotkeys.actor_retarget_gate_53bf64 = 0U;
    debug_hotkeys.battle_mode_flags_53bc24 = 0U;
    debug_hotkeys.block_53af30.fill(0U);
    debug_hotkeys.reset_gate_53bd50 = 0U;

    debug_overlay_gate = 0U;
    debug_overlay.selection_order.fill(0U);
    debug_overlay.battle_selector = -1;
    debug_overlay.initial_mode = -1;
    debug_overlay.battle_frame = 0U;

    retreat_commit.completion_gate_a = 0U;
    retreat_commit.completion_gate_b = 0U;
    retreat_commit.selected_actor_token = 0U;

    outcome_resolution.resolution_latch = 0U;
    outcome_resolution.darkening_gate = 0U;
    outcome_resolution.force_group_b_resolution = 0U;
    outcome_finalization.completion_words.fill(0U);

    vertical_shift.phase_index = 0U;
    vertical_shift.tick_limit = 0U;
}

void record_call(
    LegacyBattleGlobalResetResult& result,
    const LegacyBattleGlobalResetCallStage stage
) noexcept {
    result.call_order[result.call_count] = stage;
    ++result.call_count;
}

}  // namespace

LegacyBattleGlobalResetResult reset_legacy_battle_globals(
    LegacyBattleGlobalResetState& state,
    LegacyBattleStartupState& startup,
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleActionDispatchState& action,
    LegacyBattleGroupBFrameState& actor_frames,
    LegacyBattleDebugOverlayState& debug_overlay,
    LegacyBattleGlobalResetRuntimePort& port
) {
    LegacyBattleGlobalResetResult result;
    state.write_trace.clear();

    record_call(result, LegacyBattleGlobalResetCallStage::display_surfaces);
    result.display_surfaces =
        release_legacy_battle_display_surfaces(startup, port);

    record_call(result, LegacyBattleGlobalResetCallStage::rotation_cache);
    result.rotation_cache = release_legacy_battle_action_rotation_cache(
        startup.background_rotation_cache, port
    );

    record_call(result, LegacyBattleGlobalResetCallStage::render_resources);
    result.render_resources =
        release_legacy_battle_render_resources(startup.render_geometry, port);

    result.conditional_allocation_token = startup.reset.values_502940[0];
    if (result.conditional_allocation_token != 0U) {
        record_call(
            result, LegacyBattleGlobalResetCallStage::conditional_allocation
        );
        static_cast<void>(port.invoke_reset(
            LegacyBattleGlobalResetCall::release_conditional_allocation,
            result.conditional_allocation_token
        ));
        result.conditional_allocation_released = true;
    }

    for (u32 index = 0U; index < 232U; ++index) {
        apply_write(state, kResetWrites[index], result);
    }
    synchronize_typed_aliases(
        startup,
        final_actor,
        action,
        actor_frames,
        port.battle_message_state(),
        port.battle_terminal_latch(),
        port.battle_pair_primary_value(),
        port.battle_color_accumulation_state(),
        port.actor_metric_state(),
        port.effect_shift_state(),
        port.effect_coordinator_state(),
        port.battle_debug_hotkey_state(),
        debug_overlay,
        port.battle_debug_overlay_gate(),
        port.retreat_commit_state(),
        port.outcome_resolution_state(),
        port.outcome_finalization_state(),
        port.battle_vertical_shift_state()
    );

    record_call(
        result, LegacyBattleGlobalResetCallStage::pre_battle_resource_431960
    );
    static_cast<void>(port.invoke_reset(
        LegacyBattleGlobalResetCall::release_pre_battle_resource_431960, 0U
    ));
    record_call(
        result, LegacyBattleGlobalResetCallStage::pre_battle_resource_433010
    );
    static_cast<void>(port.invoke_reset(
        LegacyBattleGlobalResetCall::release_pre_battle_resource_433010, 0U
    ));

    record_call(result, LegacyBattleGlobalResetCallStage::all_samples);
    static_cast<void>(
        audio_video::stop_all_legacy_samples(port.sample_manager())
    );

    record_call(result, LegacyBattleGlobalResetCallStage::audio_stream);
    static_cast<void>(port.invoke_reset(
        LegacyBattleGlobalResetCall::suspend_audio_stream_485710, 0U
    ));

    record_call(
        result, LegacyBattleGlobalResetCallStage::post_reset_initialization
    );
    static_cast<void>(port.invoke_reset(
        LegacyBattleGlobalResetCall::initialize_post_reset_4776a0, 0U
    ));

    for (u32 index = 232U; index < kResetWrites.size(); ++index) {
        apply_write(state, kResetWrites[index], result);
    }
    result.return_value = 0U;
    return result;
}

}  // namespace openswd3::battle
