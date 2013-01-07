Timers used in SDT
--------------------------------------------------------------------------
memb->rem.stateTimer

addMember
   memb->rem.stateTimer.action = joinTimeoutAction;
   set_timer(&memb->rem.stateTimer, timerval_ms(memb->rem.t_ms));

createRecip
   memb->rem.t_ms = AD_HOC_TIMEOUT_ms;
   memb->rem.stateTimer.action = joinTimeoutAction;
   set_timer(&memb->rem.stateTimer, timerval_ms(memb->rem.t_ms));

joinTimeoutAction
   memb->rem.t_ms <<= 1;
   set_timer(timer, timerval_ms(memb->rem.t_ms));

rx_joinAccept
   case MS_JOINRQ:   /* this is normal - we've sen a join */
      memb->rem.mstate = MS_JOINPEND;
      memb->rem.stateTimer.action = recipTimeoutAction;
      set_timer(&memb->rem.stateTimer, RECIPROCAL_TIMEOUT(Lchan));

recipTimeoutAction
killMember
   killMember(memb, SDT_REASON_NO_RECIPROCAL, EV_JOINFAIL);

rx_ack
   case MS_JOINPEND:
      cancel_timer(&memb->rem.stateTimer);
      memb->rem.stateTimer.action = makTimeoutAction;

   memb->rem.maktries = MAK_MAX_RETRIES + 1;
   set_timer(&memb->rem.stateTimer, timerval_ms(Lchan_KEEPALIVE_ms(memb->rem.Lchan)));

setMAKs
   if (Lchan->primakHi) {
            set_timer(&memb->rem.stateTimer, Lchan_MAK_TIMEOUT(Lchan));

makTimeoutAction
   if (memb->rem.maktries == 0)
      killMember(memb, SDT_REASON_CHANNEL_EXPIRED, EV_JOINFAIL);
   else {
   	set primakHi/Lo

killMember
   cancel_timer(&memb->rem.stateTimer);

--------------------------------------------------------------------------
memb->loc.expireTimer

initLocMember

sendNAK
   memb->loc.expireTimer.action = NAKfailAction;
   set_timer(&memb->loc.expireTimer, memb_NAK_TIMEOUT(memb));

rx_outboundNAK	// NAK suppression - as though we had sent
            memb->loc.expireTimer.action = NAKfailAction;
            set_timer(&memb->loc.expireTimer, memb_NAK_TIMEOUT(memb));

NAKwrappers
      if (memb->loc.NAKstate == NS_NULL) {
         memb->loc.NAKstate = NS_SUPPRESS;
         cancel_timer(&memb->loc.expireTimer);
      memb->loc.expireTimer.action = NAKholdoffAction;
      set_timer(&memb->loc.expireTimer, timerval_ms(holdoff));

rx_wrapper
      /* sequence OK */
      while (1) {
         needack = needack || mustack(Rchan, curp->Rseq, curp->data);
         queuerxwrap(Rchan, curp);
         if ((curp = Rchan->aheadQ) == NULL) {
            /* we've caught up - cancel any NAK processing and refresh expiry */
            forEachMemb(memb, Rchan) {
               memb->loc.expireTimer.action = expireAction;
               set_timer(&memb->loc.expireTimer, timerval_s(memb->loc.params.expiry_sec));
		/* if missed seq */
         NAKwrappers(Rchan, curp->Rseq - curp->reliable);

NAKholdoffAction
   sendNAK(memb);

expireAction
/*
   Take down Rchan and reciprocal
*/
NAKfailAction
/*
   Try again up to retries
*/

killMember
   cancel_timer(&memb->loc.expireTimer);

--------------------------------------------------------------------------
Lchan->blankTimer

openChannel
   Lchan->blankTimer.action = blanktimeAction;

resendWrappers
   set_timer(&Lchan->blankTimer, timerval_ms(NAK_BLANKTIME(Lchan->params.nakholdoff)));

blanktimeAction
   Lchan->nakfirst = Lchan->naklast = 0;

note: blanking check is
   if ((first - Lchan->nakfirst) >= 0 && (last - Lchan->naklast) < 0)

--------------------------------------------------------------------------
txwrap->st.fack.rptTimer

firstACK
   inittimer(&txwrap->st.fack.rptTimer);
   txwrap->st.fack.rptTimer.action = prememberAction;
   set_timer(&txwrap->st.fack.rptTimer, timerval_ms(FIRSTACK_REPEAT_ms));

prememberAction
   if (memb->loc.mstate == MS_MEMBER) {
      if ((txwrap->st.fack.t_ms <<= 1) <= MAXACK_REPEAT_ms) {
         set_timer(timer, timerval_ms(txwrap->st.fack.t_ms));

killMember
         if (txwrap->st.fack.rptTimer.userp == memb) {
            cancel_timer(&txwrap->st.fack.rptTimer);
--------------------------------------------------------------------------
Lchan->keepalive

openChannel
   Lchan->keepalive.action = keepaliveAction;
   Lchan->ka_t_ms = Lchan_KEEPALIVE_ms(Lchan);

flushWrapper
   setMAKs
	  if (Lchan->primakHi) {
    	 Lchan->ka_t_ms >>= 1;
	  } else {
    	 if ((flags & WRAP_KEEPALIVE)
        	&& (Lchan->ka_t_ms <<= 1) > (unsigned int)Lchan_KEEPALIVE_ms(Lchan))
        	Lchan->ka_t_ms = Lchan_KEEPALIVE_ms(Lchan);
   set_timer(&Lchan->keepalive, timerval_ms(Lchan->ka_t_ms));

keepaliveAction
   emptyWrapper(Lchan, WRAP_REL_OFF | WRAP_NOAUTOACK | WRAP_KEEPALIVE);


todo:
  complete NAKfailAction expireAction
  in closeChannel cancel Lchan->keepalive and Lchan->blankTimer














