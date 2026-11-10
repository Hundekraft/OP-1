/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Plain status on black; no blue box, as requested. Native OS OFF remains last. */
static void instrument_only_badge(struct opl_self *self)
{
 native_page_fill(60,64,260,98,OPUS_COLOR_BACK);
 native_page_text_right(self,7,249,85,12,OPUS_COLOR_TEXT,"instrument only");
}
