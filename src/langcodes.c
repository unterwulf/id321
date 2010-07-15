/*
 * From: http://www.loc.gov/standards/iso639-2/php/code_list.php
 *
 * Codes for the Representation of Names of Languages
 * Codes arranged alphabetically by alpha-3/ISO 639-2 Code
 *
 * Note: ISO 639-2 is the alpha-3 code in Codes for the representation of
 * names of languages -- Part 2. There are 21 languages that have alternative
 * codes for bibliographic or terminology purposes. In those cases, each is
 * listed separately and they are designated as "B" (bibliographic) or
 * "T" (terminology). In all other cases there is only one ISO 639-2 code.
 * Multiple codes assigned to the same language are to be considered synonyms.
 * ISO 639-1 is the alpha-2 code.
 */

#include <string.h>
#include "common.h"
#include "id3v2.h"

static const char langcodes[][ID3V2_LANG_HDR_SIZE] = {
    "XXX",

    /* The non ISO 639-2 code 'XXX' is from the ID3v2 spec and meant
     * to express unknown language. Although ISO 639-2 already has 'und'
     * code for the same reason, the ID3v2 spec have introduced one more.
     * What a pity.
     *
     * From the ID3v2.4 specification:
     * "The three byte language field, present in several frames, is used to
     * describe the language of the frame's content, according to ISO-639-2
     * [ISO-639-2]. The language should be represented in lower case. If the
     * language is not known the string "XXX" should be used." */

    "aar", "abk", "ace", "ach", "ada", "ady", "afa", "afh", "afr", "ain",
    "aka", "akk", "alb", "sqi", "ale", "alg", "alt", "amh", "ang", "anp",
    "apa", "ara", "arc", "arg", "arm", "hye", "arn", "arp", "art", "arw",
    "asm", "ast", "ath", "aus", "ava", "ave", "awa", "aym", "aze", "bad",
    "bai", "bak", "bal", "bam", "ban", "baq", "eus", "bas", "bat", "bej",
    "bel", "bem", "ben", "ber", "bho", "bih", "bik", "bin", "bis", "bla",
    "bnt", "tib", "bod", "bos", "bra", "bre", "btk", "bua", "bug", "bul",
    "bur", "mya", "byn", "cad", "cai", "car", "cat", "cau", "ceb", "cel",
    "cze", "ces", "cha", "chb", "che", "chg", "chi", "zho", "chk", "chm",
    "chn", "cho", "chp", "chr", "chu", "chv", "chy", "cmc", "cop", "cor",
    "cos", "cpe", "cpf", "cpp", "cre", "crh", "crp", "csb", "cus", "wel",
    "cym", "cze", "ces", "dak", "dan", "dar", "day", "del", "den", "ger",
    "deu", "dgr", "din", "div", "doi", "dra", "dsb", "dua", "dum", "dut",
    "nld", "dyu", "dzo", "efi", "egy", "eka", "gre", "ell", "elx", "eng",
    "enm", "epo", "est", "baq", "eus", "ewe", "ewo", "fan", "fao", "per",
    "fas", "fat", "fij", "fil", "fin", "fiu", "fon", "fre", "fra", "fre",
    "fra", "frm", "fro", "frr", "frs", "fry", "ful", "fur", "gaa", "gay",
    "gba", "gem", "geo", "kat", "ger", "deu", "gez", "gil", "gla", "gle",
    "glg", "glv", "gmh", "goh", "gon", "gor", "got", "grb", "grc", "gre",
    "ell", "grn", "gsw", "guj", "gwi", "hai", "hat", "hau", "haw", "heb",
    "her", "hil", "him", "hin", "hit", "hmn", "hmo", "hrv", "hsb", "hun",
    "hup", "arm", "hye", "iba", "ibo", "ice", "isl", "ido", "iii", "ijo",
    "iku", "ile", "ilo", "ina", "inc", "ind", "ine", "inh", "ipk", "ira",
    "iro", "ice", "isl", "ita", "jav", "jbo", "jpn", "jpr", "jrb", "kaa",
    "kab", "kac", "kal", "kam", "kan", "kar", "kas", "geo", "kat", "kau",
    "kaw", "kaz", "kbd", "kha", "khi", "khm", "kho", "kik", "kin", "kir",
    "kmb", "kok", "kom", "kon", "kor", "kos", "kpe", "krc", "krl", "kro",
    "kru", "kua", "kum", "kur", "kut", "lad", "lah", "lam", "lao", "lat",
    "lav", "lez", "lim", "lin", "lit", "lol", "loz", "ltz", "lua", "lub",
    "lug", "lui", "lun", "luo", "lus", "mac", "mkd", "mad", "mag", "mah",
    "mai", "mak", "mal", "man", "mao", "mri", "map", "mar", "mas", "may",
    "msa", "mdf", "mdr", "men", "mga", "mic", "min", "mis", "mac", "mkd",
    "mkh", "mlg", "mlt", "mnc", "mni", "mno", "moh", "mon", "mos", "mao",
    "mri", "may", "msa", "mul", "mun", "mus", "mwl", "mwr", "bur", "mya",
    "myn", "myv", "nah", "nai", "nap", "nau", "nav", "nbl", "nde", "ndo",
    "nds", "nep", "new", "nia", "nic", "niu", "dut", "nld", "nno", "nob",
    "nog", "non", "nor", "nqo", "nso", "nub", "nwc", "nya", "nym", "nyn",
    "nyo", "nzi", "oci", "oji", "ori", "orm", "osa", "oss", "ota", "oto",
    "paa", "pag", "pal", "pam", "pan", "pap", "pau", "peo", "per", "fas",
    "phi", "phn", "pli", "pol", "pon", "por", "pra", "pro", "pus", "qaa",
    "que", "raj", "rap", "rar", "roa", "roh", "rom", "rum", "ron", "rum",
    "ron", "run", "rup", "rus", "sad", "sag", "sah", "sai", "sal", "sam",
    "san", "sas", "sat", "scn", "sco", "sel", "sem", "sga", "sgn", "shn",
    "sid", "sin", "sio", "sit", "sla", "slo", "slk", "slo", "slk", "slv",
    "sma", "sme", "smi", "smj", "smn", "smo", "sms", "sna", "snd", "snk",
    "sog", "som", "son", "sot", "spa", "alb", "sqi", "srd", "srn", "srp",
    "srr", "ssa", "ssw", "suk", "sun", "sus", "sux", "swa", "swe", "syc",
    "syr", "tah", "tai", "tam", "tat", "tel", "tem", "ter", "tet", "tgk",
    "tgl", "tha", "tib", "bod", "tig", "tir", "tiv", "tkl", "tlh", "tli",
    "tmh", "tog", "ton", "tpi", "tsi", "tsn", "tso", "tuk", "tum", "tup",
    "tur", "tut", "tvl", "twi", "tyv", "udm", "uga", "uig", "ukr", "umb",
    "und", "urd", "uzb", "vai", "ven", "vie", "vol", "vot", "wak", "wal",
    "war", "was", "wel", "cym", "wen", "wln", "wol", "xal", "xho", "yao",
    "yap", "yid", "yor", "ypk", "zap", "zbl", "zen", "zha", "chi", "zho",
    "znd", "zul", "zun", "zxx", "zza"
};

int is_valid_langcode(const char *langcode)
{
    size_t i;

    if (strlen(langcode) != ID3V2_LANG_HDR_SIZE)
        return 0;

    for_each (i, langcodes)
        if (!memcmp(langcode, langcodes[i], ID3V2_LANG_HDR_SIZE))
            return 1;

    return 0;
}
