#include "p2p_tool.h"
//#include "../sh_kernel.h"
#include "../p2pcommon/player_def.h"
//#include "../download/obj_manager.h"

bool init_p2p_system(SHP2PSystemParam system_param)
{
	//display use
#ifdef DEBUG
	memset(g_display_info,0,sizeof(g_display_info));
	g_display_info_len=0;
#endif

	return true;

	//return SHKernel::inst()->start(system_param);
}

bool uninit_p2p_system()
{
	//SHKernel::inst()->stop();
	return true;
}


sh_int_64 start_preload_video_data( sh_int_32 vid,enum SHVideoClarity clarity, sh_int_32 index,bool is_mytv,sh_int_32 start_range,sh_int_32 end_range )
{
	sh_int_64 unique_id = ((sh_int_64)is_mytv) << 32 | ((sh_int_64) clarity) << 33 | vid;
	/*ArgMap args_map;
	args_map["vid"] = vid;
	args_map["clarity"] =clarity;
	args_map["ismytv"] = is_mytv;
	args_map["index"] = index;
	args_map["start_range"] = start_range;
	args_map["end_range"] = end_range;
	args_map["dltype"] = SHDTYPE_PRELOAD_NEXT;

	main_thread().post(boost::bind(&DownloadManager::new_video_request,  
		DownloadManager::inst(), args_map));*/
	return unique_id;
}

void notify_buffer(sh_int_64 unique_id,sh_int_32 can_palytime)
{//通知到main_asio
	//main_ios().post(boost::bind(&DownloadManager::notify_buffer, DownloadManager::inst(), unique_id, can_palytime));
}

sh_int_64 get_video_duration(sh_int_32 vid,SHVideoClarity clarity,bool is_mytv)
{
	sh_int_64 unique_id = ((sh_int_64)is_mytv) << 32 | ((sh_int_64) clarity) << 33 | vid;
	//main_thread().post(boost::bind(&DownloadManager::fetch_video_duration, DownloadManager::inst(), unique_id, vid, clarity, is_mytv));
	return unique_id;
}

//args_map  的键名
// vid: clarity:ismytv:index:time:start_range:end_range
sh_int_64 start_request_video_data(sh_int_32 vid,SHVideoClarity clarity,sh_int_32 index,bool is_mytv, int32_t pnum)
{//通知到main_asio
	sh_int_64 unique_id = ((sh_int_64)is_mytv) << 32 | ((sh_int_64) clarity) << 33 | vid;//上层和底层都是用这个作为唯一标示
	/*ArgMap args_map;
	args_map["vid"] = vid;
	args_map["clarity"] =clarity;
	args_map["ismytv"] = is_mytv;
	args_map["index"] = index;
	args_map["dltype"] = SHDTYPE_CLIENT_PLAY;
	args_map["pnum"] = pnum;

	main_ios().post(boost::bind(&DownloadManager::new_video_request, DownloadManager::inst(), args_map));*/
	return unique_id;
}

sh_int_64 start_request_video_data_time_ex(sh_int_32 vid,SHVideoClarity clarity,bool is_mytv,double time, int32_t pnum)
{//通知到main_asio
	sh_int_64 unique_id = ((sh_int_64)is_mytv) << 32 | ((sh_int_64) clarity) << 33 | vid;
	/*ArgMap args_map;
	args_map["vid"] = vid;
	args_map["clarity"] =clarity;
	args_map["ismytv"] = is_mytv;
	args_map["time"] = time;
	args_map["dltype"] = SHDTYPE_CLIENT_PLAY;
	args_map["pnum"] = pnum;

	main_thread().post(boost::bind(&DownloadManager::new_video_request,  
		DownloadManager::inst(), args_map));*/
	return   unique_id;
}

void notify_play_num( sh_int_64 unique_id, sh_int_32 num )
{//通知到main_asio
	//main_thread().post(boost::bind(&DownloadManager::notify_play_num, DownloadManager::inst(), unique_id, num));
}

sh_int_64 start_request_video_data_range(sh_int_32 vid,SHVideoClarity clarity,
													sh_int_32 index,bool is_mytv,sh_int_32 start_range,sh_int_32 end_range)
{//通知到main_asio
	sh_int_64 unique_id = ((sh_int_64)is_mytv) << 32 | ((sh_int_64) clarity) << 33 | vid;
	/*ArgMap args_map;
	args_map["vid"] = vid;
	args_map["clarity"] =clarity;
	args_map["ismytv"] = is_mytv;
	args_map["index"] = index;
	args_map["start_range"] = start_range;
	args_map["end_range"] = end_range;
	args_map["dltype"] = SHDTYPE_CLIENT_PLAY;

	main_thread().post(boost::bind(&DownloadManager::new_video_request,  
		DownloadManager::inst(), args_map));*/
	return unique_id;
}