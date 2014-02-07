GRPATH='../..'
#cp digital_ofdm_frame_sink.cc $GRPATH/gr-digital/lib/
#cp digital_ofdm_frame_sink.h  $GRPATH/gr-digital/include/
cp original_gr_peak_detector_fb.cc $GRPATH/build/gnuradio-core/src/lib/gengen/gr_peak_detector_fb.cc
cp original_gr_peak_detector_fb.h $GRPATH/build/gnuradio-core/src/lib/gengen/gr_peak_detector_fb.h
#cd ../../build
#make
#sudo make install
