//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_CRYPTO_WHIRLPOOL
#define COGS_HEADER_CRYPTO_WHIRLPOOL


#include "cogs/crypto/serial_hash.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/mem/endian.hpp"


namespace cogs {
namespace crypto {

#ifdef DOXYGEN


/// @ingroup Crypto
/// @brief Whirlpool Cryptographic Hash Algorithm
class whirlpool : public hash
{
};


#else


class whirlpool : public serial_hash<512, 512, 64, endian_t::big, 512>
{
private:
	typedef serial_hash<512, 512, 64, endian_t::big, 512> base_t;

	fixed_integer<false, 256> m_bitCount;
	uint64_t	m_state[8];

	static uint64_t r(const uint64_t* a, uint8_t n)
	{
		static constexpr uint64_t t[256] =
		{
			0x18186018C07830D8ULL, 0x23238C2305AF4626ULL, 0xC6C63FC67EF991B8ULL, 0xE8E887E8136FCDFBULL,
			0x878726874CA113CBULL, 0xB8B8DAB8A9626D11ULL, 0x0101040108050209ULL, 0x4F4F214F426E9E0DULL,
			0x3636D836ADEE6C9BULL, 0xA6A6A2A6590451FFULL, 0xD2D26FD2DEBDB90CULL, 0xF5F5F3F5FB06F70EULL,
			0x7979F979EF80F296ULL, 0x6F6FA16F5FCEDE30ULL, 0x91917E91FCEF3F6DULL, 0x52525552AA07A4F8ULL,
			0x60609D6027FDC047ULL, 0xBCBCCABC89766535ULL, 0x9B9B569BACCD2B37ULL, 0x8E8E028E048C018AULL,
			0xA3A3B6A371155BD2ULL, 0x0C0C300C603C186CULL, 0x7B7BF17BFF8AF684ULL, 0x3535D435B5E16A80ULL,
			0x1D1D741DE8693AF5ULL, 0xE0E0A7E05347DDB3ULL, 0xD7D77BD7F6ACB321ULL, 0xC2C22FC25EED999CULL,
			0x2E2EB82E6D965C43ULL, 0x4B4B314B627A9629ULL, 0xFEFEDFFEA321E15DULL, 0x575741578216AED5ULL,
			0x15155415A8412ABDULL, 0x7777C1779FB6EEE8ULL, 0x3737DC37A5EB6E92ULL, 0xE5E5B3E57B56D79EULL,
			0x9F9F469F8CD92313ULL, 0xF0F0E7F0D317FD23ULL, 0x4A4A354A6A7F9420ULL, 0xDADA4FDA9E95A944ULL,
			0x58587D58FA25B0A2ULL, 0xC9C903C906CA8FCFULL, 0x2929A429558D527CULL, 0x0A0A280A5022145AULL,
			0xB1B1FEB1E14F7F50ULL, 0xA0A0BAA0691A5DC9ULL, 0x6B6BB16B7FDAD614ULL, 0x85852E855CAB17D9ULL,
			0xBDBDCEBD8173673CULL, 0x5D5D695DD234BA8FULL, 0x1010401080502090ULL, 0xF4F4F7F4F303F507ULL,
			0xCBCB0BCB16C08BDDULL, 0x3E3EF83EEDC67CD3ULL, 0x0505140528110A2DULL, 0x676781671FE6CE78ULL,
			0xE4E4B7E47353D597ULL, 0x27279C2725BB4E02ULL, 0x4141194132588273ULL, 0x8B8B168B2C9D0BA7ULL,
			0xA7A7A6A7510153F6ULL, 0x7D7DE97DCF94FAB2ULL, 0x95956E95DCFB3749ULL, 0xD8D847D88E9FAD56ULL,
			0xFBFBCBFB8B30EB70ULL, 0xEEEE9FEE2371C1CDULL, 0x7C7CED7CC791F8BBULL, 0x6666856617E3CC71ULL,
			0xDDDD53DDA68EA77BULL, 0x17175C17B84B2EAFULL, 0x4747014702468E45ULL, 0x9E9E429E84DC211AULL,
			0xCACA0FCA1EC589D4ULL, 0x2D2DB42D75995A58ULL, 0xBFBFC6BF9179632EULL, 0x07071C07381B0E3FULL,
			0xADAD8EAD012347ACULL, 0x5A5A755AEA2FB4B0ULL, 0x838336836CB51BEFULL, 0x3333CC3385FF66B6ULL,
			0x636391633FF2C65CULL, 0x02020802100A0412ULL, 0xAAAA92AA39384993ULL, 0x7171D971AFA8E2DEULL,
			0xC8C807C80ECF8DC6ULL, 0x19196419C87D32D1ULL, 0x494939497270923BULL, 0xD9D943D9869AAF5FULL,
			0xF2F2EFF2C31DF931ULL, 0xE3E3ABE34B48DBA8ULL, 0x5B5B715BE22AB6B9ULL, 0x88881A8834920DBCULL,
			0x9A9A529AA4C8293EULL, 0x262698262DBE4C0BULL, 0x3232C8328DFA64BFULL, 0xB0B0FAB0E94A7D59ULL,
			0xE9E983E91B6ACFF2ULL, 0x0F0F3C0F78331E77ULL, 0xD5D573D5E6A6B733ULL, 0x80803A8074BA1DF4ULL,
			0xBEBEC2BE997C6127ULL, 0xCDCD13CD26DE87EBULL, 0x3434D034BDE46889ULL, 0x48483D487A759032ULL,
			0xFFFFDBFFAB24E354ULL, 0x7A7AF57AF78FF48DULL, 0x90907A90F4EA3D64ULL, 0x5F5F615FC23EBE9DULL,
			0x202080201DA0403DULL, 0x6868BD6867D5D00FULL, 0x1A1A681AD07234CAULL, 0xAEAE82AE192C41B7ULL,
			0xB4B4EAB4C95E757DULL, 0x54544D549A19A8CEULL, 0x93937693ECE53B7FULL, 0x222288220DAA442FULL,
			0x64648D6407E9C863ULL, 0xF1F1E3F1DB12FF2AULL, 0x7373D173BFA2E6CCULL, 0x12124812905A2482ULL,
			0x40401D403A5D807AULL, 0x0808200840281048ULL, 0xC3C32BC356E89B95ULL, 0xECEC97EC337BC5DFULL,
			0xDBDB4BDB9690AB4DULL, 0xA1A1BEA1611F5FC0ULL, 0x8D8D0E8D1C830791ULL, 0x3D3DF43DF5C97AC8ULL,
			0x97976697CCF1335BULL, 0x0000000000000000ULL, 0xCFCF1BCF36D483F9ULL, 0x2B2BAC2B4587566EULL,
			0x7676C57697B3ECE1ULL, 0x8282328264B019E6ULL, 0xD6D67FD6FEA9B128ULL, 0x1B1B6C1BD87736C3ULL,
			0xB5B5EEB5C15B7774ULL, 0xAFAF86AF112943BEULL, 0x6A6AB56A77DFD41DULL, 0x50505D50BA0DA0EAULL,
			0x45450945124C8A57ULL, 0xF3F3EBF3CB18FB38ULL, 0x3030C0309DF060ADULL, 0xEFEF9BEF2B74C3C4ULL,
			0x3F3FFC3FE5C37EDAULL, 0x55554955921CAAC7ULL, 0xA2A2B2A2791059DBULL, 0xEAEA8FEA0365C9E9ULL,
			0x656589650FECCA6AULL, 0xBABAD2BAB9686903ULL, 0x2F2FBC2F65935E4AULL, 0xC0C027C04EE79D8EULL,
			0xDEDE5FDEBE81A160ULL, 0x1C1C701CE06C38FCULL, 0xFDFDD3FDBB2EE746ULL, 0x4D4D294D52649A1FULL,
			0x92927292E4E03976ULL, 0x7575C9758FBCEAFAULL, 0x06061806301E0C36ULL, 0x8A8A128A249809AEULL,
			0xB2B2F2B2F940794BULL, 0xE6E6BFE66359D185ULL, 0x0E0E380E70361C7EULL, 0x1F1F7C1FF8633EE7ULL,
			0x6262956237F7C455ULL, 0xD4D477D4EEA3B53AULL, 0xA8A89AA829324D81ULL, 0x96966296C4F43152ULL,
			0xF9F9C3F99B3AEF62ULL, 0xC5C533C566F697A3ULL, 0x2525942535B14A10ULL, 0x59597959F220B2ABULL,
			0x84842A8454AE15D0ULL, 0x7272D572B7A7E4C5ULL, 0x3939E439D5DD72ECULL, 0x4C4C2D4C5A619816ULL,
			0x5E5E655ECA3BBC94ULL, 0x7878FD78E785F09FULL, 0x3838E038DDD870E5ULL, 0x8C8C0A8C14860598ULL,
			0xD1D163D1C6B2BF17ULL, 0xA5A5AEA5410B57E4ULL, 0xE2E2AFE2434DD9A1ULL, 0x616199612FF8C24EULL,
			0xB3B3F6B3F1457B42ULL, 0x2121842115A54234ULL, 0x9C9C4A9C94D62508ULL, 0x1E1E781EF0663CEEULL,
			0x4343114322528661ULL, 0xC7C73BC776FC93B1ULL, 0xFCFCD7FCB32BE54FULL, 0x0404100420140824ULL,
			0x51515951B208A2E3ULL, 0x99995E99BCC72F25ULL, 0x6D6DA96D4FC4DA22ULL, 0x0D0D340D68391A65ULL,
			0xFAFACFFA8335E979ULL, 0xDFDF5BDFB684A369ULL, 0x7E7EE57ED79BFCA9ULL, 0x242490243DB44819ULL,
			0x3B3BEC3BC5D776FEULL, 0xABAB96AB313D4B9AULL, 0xCECE1FCE3ED181F0ULL, 0x1111441188552299ULL,
			0x8F8F068F0C890383ULL, 0x4E4E254E4A6B9C04ULL, 0xB7B7E6B7D1517366ULL, 0xEBEB8BEB0B60CBE0ULL,
			0x3C3CF03CFDCC78C1ULL, 0x81813E817CBF1FFDULL, 0x94946A94D4FE3540ULL, 0xF7F7FBF7EB0CF31CULL,
			0xB9B9DEB9A1676F18ULL, 0x13134C13985F268BULL, 0x2C2CB02C7D9C5851ULL, 0xD3D36BD3D6B8BB05ULL,
			0xE7E7BBE76B5CD38CULL, 0x6E6EA56E57CBDC39ULL, 0xC4C437C46EF395AAULL, 0x03030C03180F061BULL,
			0x565645568A13ACDCULL, 0x44440D441A49885EULL, 0x7F7FE17FDF9EFEA0ULL, 0xA9A99EA921374F88ULL,
			0x2A2AA82A4D825467ULL, 0xBBBBD6BBB16D6B0AULL, 0xC1C123C146E29F87ULL, 0x53535153A202A6F1ULL,
			0xDCDC57DCAE8BA572ULL, 0x0B0B2C0B58271653ULL, 0x9D9D4E9D9CD32701ULL, 0x6C6CAD6C47C1D82BULL,
			0x3131C43195F562A4ULL, 0x7474CD7487B9E8F3ULL, 0xF6F6FFF6E309F115ULL, 0x464605460A438C4CULL,
			0xACAC8AAC092645A5ULL, 0x89891E893C970FB5ULL, 0x14145014A04428B4ULL, 0xE1E1A3E15B42DFBAULL,
			0x16165816B04E2CA6ULL, 0x3A3AE83ACDD274F7ULL, 0x6969B9696FD0D206ULL, 0x09092409482D1241ULL,
			0x7070DD70A7ADE0D7ULL, 0xB6B6E2B6D954716FULL, 0xD0D067D0CEB7BD1EULL, 0xEDED93ED3B7EC7D6ULL,
			0xCCCC17CC2EDB85E2ULL, 0x424215422A578468ULL, 0x98985A98B4C22D2CULL, 0xA4A4AAA4490E55EDULL,
			0x2828A0285D885075ULL, 0x5C5C6D5CDA31B886ULL, 0xF8F8C7F8933FED6BULL, 0x8686228644A411C2ULL
		};

		uint8_t i = (uint8_t)(a[n] >> 56);
		uint64_t b = t[i];
		i = (uint8_t)(a[(n + 7) & 0x07] >> 48);
		b ^= bit_rotate_right(t[i], 8);
		i = (uint8_t)(a[(n + 6) & 0x07] >> 40);
		b ^= bit_rotate_right(t[i], 16);
		i = (uint8_t)(a[(n + 5) & 0x07] >> 32);
		b ^= bit_rotate_right(t[i], 24);
		i = (uint8_t)(a[(n + 4) & 0x07] >> 24);
		b ^= bit_rotate_right(t[i], 32);
		i = (uint8_t)(a[(n + 3) & 0x07] >> 16);
		b ^= bit_rotate_right(t[i], 40);
		i = (uint8_t)(a[(n + 2) & 0x07] >> 8);
		b ^= bit_rotate_right(t[i], 48);
		i = (uint8_t)(a[(n + 1) & 0x07]);
		b ^= bit_rotate_right(t[i], 56);
		return b;
	}

	void process_digit()
	{
		m_state[base_t::m_blockProgress] = base_t::m_curDigit;
		base_t::advance_digit();
	}

	void process_block()
	{
		static constexpr uint64_t roundConstants[10] =
		{
			0x1823C6E887B8014FULL,
			0x36A6D2F5796F9152ULL,
			0x60BC9B8EA30C7B35ULL,
			0x1DE0D7C22E4BFE57ULL,
			0x157737E59FF04ADAULL,
			0x58C9290AB1A06B85ULL,
			0xBD5D10F4CB3E0567ULL,
			0xE427418BA77D95D8ULL,
			0xFBEE7C66DD17479EULL,
			0xCA2DBF07AD5A8333ULL
		};

		m_bitCount += stride_bits;

		uint64_t tmpResult[8];
		uint64_t tmpState[8];
		uint64_t tmp[8];

		tmpResult[0] = m_result[0];
		tmpResult[1] = m_result[1];
		tmpResult[2] = m_result[2];
		tmpResult[3] = m_result[3];
		tmpResult[4] = m_result[4];
		tmpResult[5] = m_result[5];
		tmpResult[6] = m_result[6];
		tmpResult[7] = m_result[7];

		tmpState[0] = m_state[0] ^ tmpResult[0];
		tmpState[1] = m_state[1] ^ tmpResult[1];
		tmpState[2] = m_state[2] ^ tmpResult[2];
		tmpState[3] = m_state[3] ^ tmpResult[3];
		tmpState[4] = m_state[4] ^ tmpResult[4];
		tmpState[5] = m_state[5] ^ tmpResult[5];
		tmpState[6] = m_state[6] ^ tmpResult[6];
		tmpState[7] = m_state[7] ^ tmpResult[7];

		for (uint8_t i = 0; i < 10; i++)
		{  
			tmp[0] = r(tmpResult, 0) ^ roundConstants[i];
			tmp[1] = r(tmpResult, 1);
			tmp[2] = r(tmpResult, 2);
			tmp[3] = r(tmpResult, 3);
			tmp[4] = r(tmpResult, 4);
			tmp[5] = r(tmpResult, 5);
			tmp[6] = r(tmpResult, 6);
			tmp[7] = r(tmpResult, 7);

			tmpResult[0] = tmp[0];
			tmpResult[1] = tmp[1];
			tmpResult[2] = tmp[2];
			tmpResult[3] = tmp[3];
			tmpResult[4] = tmp[4];
			tmpResult[5] = tmp[5];
			tmpResult[6] = tmp[6];
			tmpResult[7] = tmp[7];
		
			tmp[0] = r(tmpState, 0) ^ tmpResult[0];
			tmp[1] = r(tmpState, 1) ^ tmpResult[1];
			tmp[2] = r(tmpState, 2) ^ tmpResult[2];
			tmp[3] = r(tmpState, 3) ^ tmpResult[3];
			tmp[4] = r(tmpState, 4) ^ tmpResult[4];
			tmp[5] = r(tmpState, 5) ^ tmpResult[5];
			tmp[6] = r(tmpState, 6) ^ tmpResult[6];
			tmp[7] = r(tmpState, 7) ^ tmpResult[7];

			tmpState[0] = tmp[0];
			tmpState[1] = tmp[1];
			tmpState[2] = tmp[2];
			tmpState[3] = tmp[3];
			tmpState[4] = tmp[4];
			tmpState[5] = tmp[5];
			tmpState[6] = tmp[6];
			tmpState[7] = tmp[7];
		} 
		
		m_result[0] ^= tmpState[0] ^ m_state[0];
		m_result[1] ^= tmpState[1] ^ m_state[1];
		m_result[2] ^= tmpState[2] ^ m_state[2];
		m_result[3] ^= tmpState[3] ^ m_state[3];
		m_result[4] ^= tmpState[4] ^ m_state[4];
		m_result[5] ^= tmpState[5] ^ m_state[5];
		m_result[6] ^= tmpState[6] ^ m_state[6];
		m_result[7] ^= tmpState[7] ^ m_state[7];
	}

	void terminate()
	{
		fixed_integer<false, 256> bitCount = m_bitCount + (base_t::m_blockProgress * 64) + base_t::m_digitProgress;

		// Add terminating 0x80 byte.  Might cause this block to process, if last byte.
		add_byte(0x80);

		// Finish a current digit in progress, if any
		if (m_digitProgress != 0)
			process_digit();

		// We need 4 digits for the length.  If not enough space, add padding to this block, start a new one.
		while (m_blockProgress != stride_digits - 4)
			process_digit();

		m_state[stride_digits - 4] = (uint64_t)(bitCount >> 192).get_int();
		m_state[stride_digits - 3] = (uint64_t)(bitCount >> 128).get_int();
		m_state[stride_digits - 2] = (uint64_t)(bitCount >> 64).get_int();
		m_state[stride_digits - 1] = (uint64_t)bitCount.get_int();
		process_block();
	}

public:
	whirlpool()
		: base_t([&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = 0;
		for (size_t i = 0; i < result_digits; i++)
			m_result[i] = 0;
	}

	whirlpool(const whirlpool& src)
		: base_t(src, [&]() { process_digit(); }, [&]() { process_block(); }, [&]() { terminate(); })
	{
		m_bitCount = src.m_bitCount;
		for (size_t i = 0; i < m_blockProgress; i++)
			m_state[i] = src.m_state[i];
	}

	whirlpool& operator=(const whirlpool& src)
	{
		m_bitCount = src.m_bitCount;
		base_t::operator=(src);
		for (size_t i = 0; i < m_blockProgress; i++)
			m_state[i] = src.m_state[i];
		return *this;
	}
};


#endif


}
}


#endif
