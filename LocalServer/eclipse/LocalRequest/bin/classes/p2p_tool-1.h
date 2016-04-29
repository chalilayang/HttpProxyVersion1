#ifndef __P2P_TOOL_H__
#define __P2P_TOOL_H__
#include "../p2pcommon/sh_p2p_system_define.h"
#include "../p2pcommon/base/common.h"

//初始化p2p系统
bool init_p2p_system(SHP2PSystemParam system_param/*,SHP2pSystemNofity system_notify*/); 

//反初始化p2p系统
bool uninit_p2p_system();


//预加载请求
sh_int_64 start_preload_video_data(sh_int_32 vid,enum SHVideoClarity clarity,sh_int_32 index,bool is_mytv,sh_int_32 start_range,sh_int_32 end_range); //????

/*通知缓冲API sh_int_32 can_palytime		剩余数据可播放时间*/	
void notify_buffer(sh_int_64 unique_id,sh_int_32 can_palytime);

//
sh_int_64 get_video_duration(sh_int_32 vid,enum SHVideoClarity clarity,bool is_mytv); //????

//
sh_int_64 start_request_video_data(sh_int_32 vid,enum SHVideoClarity clarity,sh_int_32 index,bool is_mytv, sh_int_32 pnum);

sh_int_64 start_request_video_data_time_ex(sh_int_32 vid,enum SHVideoClarity clarity,bool is_mytv,double time, sh_int_32 pnum);

/*通知正在播放的段号*/
void notify_play_num(sh_int_64 unique_id,  sh_int_32 num);

//以range方式请求， 使用时机：在播放请求中断后，从断点处开始继续下载， end_range为-1表示到结尾
sh_int_64 start_request_video_data_range(sh_int_32 vid,enum SHVideoClarity clarity,sh_int_32 index,bool is_mytv,sh_int_32 start_range,sh_int_32 end_range); 


#endif //__P2P_TOOL_H__