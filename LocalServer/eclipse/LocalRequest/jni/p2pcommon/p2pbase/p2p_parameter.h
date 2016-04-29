#ifndef P2P_PARAMETER_H_
#define P2P_PARAMETER_H_

#include "../api/sh_p2p_system_define.h"
#include <string>

class P2pParameter
{
public:
  long				start_section_cdn_num_;//开始段下载的CDN数目，默认是三个
  long				send_time_out_;	                    //发送超时 单位 s
  long				peer_speed_less_use_cdn_;//下载预加载时PEER速度小于此值时开始使用CDN（KB/s）
  long				use_peer_cdn_speed_less_;//当CDN速度小于usePeerWhenCdnSpeedLess KB/s时使用PEER

  long				urgent_to_p2p_time_;
  long				urgent_top_p2p_num_;
  long				urgent_to_state_time_;
  long				urgent_p2p_to_http_time_;
  long				urgent_p2p_to_http_time_delay_;
  long				urgent_p2p_to_http_speed_;
  long				urgent_mix_to_p2p_speed_;
  long				play_to_state_time_;
  long				play_to_http_speed_;
  long				play_http_to_p2p_time_;
  long				play_p2p_to_http_speed_;
  long				play_mix_to_p2p_speed_;
  long				speed_limit_for_working_;
  long				speed_limit_for_stay_;
  long				speed_limit_for_full_screen_;
  long				notify_buffer_time_;
  long				check_connect_status_interval; 
  long				urgent_coefficient_;
  long				force_to_http_time;
  long				p2p_to_http_peer_num_;
  long				http_speed_coefficient_;
  long				refetch_peer_;

  long              gaps_seconds_;
  long              supply_peers_num_;

  long              max_peer_conn_;
  long              prior_value_factor_;

  long              http_startup_speed_factor_;
  long              p2p_speed_limit_factor_;

  long              share_file_;
  long              max_upload_speed_;
  long              max_up_connect_;

  long				download_p2p_to_http_min_speed_;
  long				download_p2p_to_http_speed_when_few_peer_;
  long               download_http_to_p2p_when_http_speed_less_;

  long				p2p_speed_leave_only_p2p_in_comb_status_;
  long				p2p_speed_leave_only_http_in_comb_status_;
  
  long				p2p_feed_hunger_stategy_;
  long				p2p_only_feed_download_hunger_;

  long               unblock_punch_;
  long              urgent_concurrent_download_mode_;	
  long				urgent_preload_time_;

#ifdef ENABLE_FLASH_P2P
  long				p2p_download_peer_count_;
  long				max_flash_peer_connection_;
  long				enable_flash_p2p_;
  long				flash_target_speed; //KiByte from navigation
  long              max_piece_per_second_;
#endif // #ifdef ENABLE_FLASH_P2P

#ifdef ENABLE_LUA_STATE_MACHINE
  long				enable_lua_state_machine_;
  long				lua_state_machine_count_;
  long				lua_state_machine_version_;
  long				force_overwrite_lua_state_machine_;
  std::string		lua_state_machine_url_;
#endif // #ifdef ENABLE_LUA_STATE_MACHINE
  
  long               default_sp_code_;
  long               default_city_code_;
  long               if_vrs_iplocate_;
  
#ifdef ENABLE_GATEWAY
  long               gateway_reconnect_interval_;
  long               gateway_update_interval_;
  long               gateway_disconnect_timeout_;
#endif // #ifdef ENABLE_GATEWAY

#ifdef ENABLE_CACHE_PEER
  long               enable_cache_ifox_peer_;
  long               enable_cache_flash_peer_;
  long               cached_peer_life_time_;
#endif // #ifdef ENABLE_CACHE_PEER

  long               p2p_sync_non_block_read_count_;
  P2pParameter();
};

extern P2pParameter g_p2p_param;

enum SHCDNPtType 
{
    kSHPlatform_None = -1,
    kSHPlatform_PC = 1,
    kSHPlatform_IPAD,
    kSHPlatform_IPHONE,     
    kSHPlatform_AndroidPad,
    kSHPlatform_AndroidPhone,
    kSHPlatform_AndroidTV,
    kSHPlatform_MAC = 10
};

typedef enum SysParamName
{
	kSysParam_start_section_cdn_num = 0,
	kSysParam_send_time_out,
	kSysParam_peer_speed_less_use_cdn,
	kSysParam_use_peer_cdn_speed_less,
	kSysParam_urgent_to_p2p_time,
	kSysParam_urgent_top_p2p_num,
	kSysParam_urgent_to_state_time,
	kSysParam_urgent_p2p_to_http_time,
	kSysParam_urgent_p2p_to_http_time_delay,
	kSysParam_urgent_p2p_to_http_speed,
	kSysParam_urgent_mix_to_p2p_speed,
	kSysParam_play_to_state_time,
	kSysParam_play_to_http_speed,
	kSysParam_play_http_to_p2p_time,
	kSysParam_play_p2p_to_http_speed,
	kSysParam_play_mix_to_p2p_speed,
	kSysParam_speed_limit_for_working,
	kSysParam_speed_limit_for_stay,
	kSysParam_speed_limit_for_full_screen,
	kSysParam_notify_buffer_time,
	kSysParam_check_connect_status_interval,
	kSysParam_urgent_coefficient,
	kSysParam_force_to_http_time,
	kSysParam_p2p_to_http_peer_num,
	kSysParam_http_speed_coefficient,
	kSysParam_refetch_peer,
	kSysParam_gaps_seconds,
	kSysParam_supply_peers_num,
	kSysParam_max_peer_conn,
	kSysParam_prior_value_factor,
	kSysParam_http_startup_speed_factor,
	kSysParam_p2p_speed_limit_factor,
	kSysParam_share_file,
	kSysParam_max_upload_speed,
	kSysParam_max_up_connect,
	kSysParam_download_p2p_to_http_min_speed,
	kSysParam_download_p2p_to_http_speed_when_few_peer,
	kSysParam_download_http_to_p2p_when_http_speed_less,
	kSysParam_p2p_speed_leave_only_p2p_in_comb_status,
	kSysParam_p2p_speed_leave_only_http_in_comb_status,
	kSysParam_p2p_feed_hunger_stategy,
	kSysParam_p2p_only_feed_download_hunger,
	kSysParam_unblock_punch,
	kSysParam_urgent_concurrent_download_mode,
	kSysParam_urgent_preload_time,
#ifdef ENABLE_FLASH_P2P
	kSysParam_p2p_download_peer_count,
	kSysParam_max_flash_peer_connection,
	kSysParam_enable_flash_p2p,
	kSysParam_flash_target_speed,
#endif // #ifdef ENABLE_FLASH_P2P

#ifdef ENABLE_LUA_STATE_MACHINE
	kSysParam_enable_lua_state_machine,
	kSysParam_lua_state_machine_count,
	kSysParam_lua_state_machine_version,
	kSysParam_force_overwrite_lua_state_machine_,
#endif // #ifdef ENABLE_LUA_STATE_MACHINE
	kSysParam_count
}SysParamName;

extern SHCDNPtType g_pt_map[PLATFORM_ANDROID_TV];

#endif
