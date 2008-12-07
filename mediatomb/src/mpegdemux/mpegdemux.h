/* 
 * MT 
 * 
 * */


#ifndef __MPEGDEMUX_H__
#define __MPEGDEMUX_H__

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Remuxes an MPEG2 stream and keeps only the video and audio streams
/// that are given by the ID.
//
/// \param fd_in Already opened file descriptor for reading the input stream.
/// \param fd_out Already opeend file descriptor for writing the remuxed data.
/// \param keep_video_id id of the video that should be kept.
/// \param keep_audio_id id of the audio stream taht should be kept.
/// \return 0 on success.

int remux_mpeg(int fd_in, int fd_out, 
               unsigned char keep_video_id, 
               unsigned char keep_audio_id);

#ifdef __cplusplus
}  // extern "C"
#endif


#endif
