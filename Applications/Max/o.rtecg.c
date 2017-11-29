/*

*/


#define OMAX_DOC_NAME "o.rtecg"
#define OMAX_DOC_SHORT_DESC "Real-time processing of an ECG signal"
#define OMAX_DOC_LONG_DESC "Real-time processing of an ECG signal"
#define OMAX_DOC_INLETS_DESC (char *[]){"FullPacket containing /time and /ecg"}
#define OMAX_DOC_OUTLETS_DESC (char *[]){"FullPacket"}
#define OMAX_DOC_SEEALSO (char *[]){"nothing"}

//#include "odot_version.h"
#include "ext.h"
#include "ext_obex.h"
#include "ext_critical.h"
#include "ext_obex_util.h"
#include "osc.h"
#include "osc_mem.h"
#include "o.h"
#include "omax_util.h"
#include "omax_doc.h"
#include "omax_dict.h"

#include "rtecg.h"
#include "rtecg_filter.h"
#include "rtecg_pantompkins.h"

typedef struct _oO{
	t_object ob;
	void *outlet;
    rtecg_ptlp lp;
    rtecg_pthp hp;
    rtecg_ptd d;
    rtecg_pti mwi;
    rtecg_pk pkf;
    rtecg_pk pki;
    rtecg_pt pts;
    t_osc_timetag tlst[RTECG_FS * 2];
    int tptr;
} t_oO;

void *oO_class;


void oO_fullPacket(t_oO *x, t_symbol *msg, int argc, t_atom *argv)
{
	OMAX_UTIL_GET_LEN_AND_PTR;
	t_osc_msg_ar_s *msgs_ecg_s = osc_bundle_s_lookupAddress(len, ptr, "/ecg", 1);
	if(osc_message_array_s_getLen(msgs_ecg_s) == 0){
		return;
	}
	t_osc_msg_s *msg_ecg_s = osc_message_array_s_get(msgs_ecg_s, 0);
    rtecg_float a0 = 1.0;
    x->lp = rtecg_ptlp_hx0(x->lp, a0);
    x->hp = rtecg_pthp_hx0(x->hp, rtecg_ptlp_y0(x->lp));
    x->d = rtecg_ptd_hx0(x->d, rtecg_pthp_y0(x->hp));
    x->mwi = rtecg_pti_hx0(x->mwi, rtecg_ptd_y0(x->d));
    // mark peaks
    x->pkf = rtecg_pk_mark(x->pkf, rtecg_pthp_y0(x->hp));
    x->pki = rtecg_pk_mark(x->pki, rtecg_pti_y0(x->mwi));
    // classify
    int buflen = 1;//1200;
    char buf[buflen];
    buf[0] = 0;
    x->pts = rtecg_pt_process(x->pts, rtecg_pk_y0(x->pkf) * rtecg_pk_xm82(x->pkf), rtecg_pk_maxslope(x->pkf), rtecg_pk_y0(x->pki) * rtecg_pk_xm82(x->pki), rtecg_pk_maxslope(x->pki), buf, buflen, 0);
    if(x->pts.searchback){
        buf[0] = 0;
        x->pts = rtecg_pt_searchback(x->pts, buf, buflen, 0);
    }
}

void oO_anything(t_oO *x, t_symbol *msg, int argc, t_atom *argv)
{
}

void oO_bang(t_oO *x)
{
}

OMAX_DICT_DICTIONARY(t_oO, x, oO_fullPacket);

void oO_doc(t_oO *x)
{
	omax_doc_outletDoc(x->outlet);
}

void oO_assist(t_oO *x, void *b, long io, long num, char *buf)
{
	omax_doc_assist(io, num, buf);
}

void oO_free(t_oO *x)
{
}

void *oO_new(t_symbol *msg, short argc, t_atom *argv)
{
	t_oO *x = NULL;
	if((x = (t_oO *)object_alloc(oO_class))){
		x->outlet = outlet_new((t_object *)x, "FullPacket");
	}
	return x;
}

int main(void)
{
	t_class *c = class_new("o.rtecg", (method)oO_new, (method)oO_free, sizeof(t_oO), 0L, A_GIMME, 0);
	//class_addmethod(c, (method)oO_fullPacket, "FullPacket", A_LONG, A_LONG, 0);
	class_addmethod(c, (method)oO_fullPacket, "FullPacket", A_GIMME, 0);
	class_addmethod(c, (method)oO_assist, "assist", A_CANT, 0);
	class_addmethod(c, (method)oO_doc, "doc", 0);
	//class_addmethod(c, (method)oO_bang, "bang", 0);
	//class_addmethod(c, (method)oO_anything, "anything", A_GIMME, 0);
		class_addmethod(c, (method)omax_dict_dictionary, "dictionary", A_GIMME, 0);
	//class_addmethod(c, (method)odot_version, "version", 0);
	
	class_register(CLASS_BOX, c);
	oO_class = c;

	common_symbols_init();

	//ODOT_PRINT_VERSION;

	srandomdev();
	return 0;
}
/*
t_max_err oO_notify(t_oO *x, t_symbol *s, t_symbol *msg, void *sender, void *data){
	t_symbol *attrname;

        if(msg == gensym("attr_modified")){
		attrname = (t_symbol *)object_method((t_object *)data, gensym("getname"));
	}
	return MAX_ERR_NONE;
}
*/
