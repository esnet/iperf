/*
 * iperf, Copyright (c) 2014, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */

/**
 * \file   iperf_udp.h
 * \brief  iperf3 UDP 테스트용 함수 선언
 * \author 이준상
 * \date   2026-11-09
 *
 * UDP는 연결 개념이 없어서, 첫 데이터그램 교환으로
 * 클라이언트-서버 간 스트림을 맺는 방식 사용.
 *   - 서버 측
 *     -# iperf_udp_listen() : UDP 리스닝 소켓 생성
 *     -# iperf_udp_accept() : 클라이언트의 첫 데이터그램 수신 후 연결
 *   - 클라이언트 측
 *     -# iperf_udp_connect() : 서버에 첫 데이터그램 전송 후 응답 대기
 *   - 공통
 *     -# iperf_udp_init() : UDP 테스트 초기화
 *     -# iperf_udp_send() / iperf_udp_recv() : 데이터 송수신
 */
#ifndef __IPERF_UDP_H
#define __IPERF_UDP_H

/**
 * \brief UDP 스트림에서 데이터그램 수신
 *
 * 패킷 헤더의 \e 시퀀스 번호와 \e 타임스탬프로
 * 패킷 손실, 순서 뒤바뀜, 지터(jitter) 계산.
 *
 * \param sp 데이터를 받을 스트림
 * \return 수신한 바이트 수, 오류 시 음수
 */
int iperf_udp_recv(struct iperf_stream *sp);

/**
 * \brief UDP 스트림으로 데이터그램 전송
 *
 * 각 패킷 앞부분에 전송 시각과 시퀀스 번호를 기록해 전송.
 *
 * \param sp 데이터를 보낼 스트림
 * \return 전송한 바이트 수, 오류 시 음수
 */
int iperf_udp_send(struct iperf_stream *sp) /* __attribute__((hot)) */;

/**
 * \brief 서버 측에서 새 UDP 스트림 수락
 *
 * 클라이언트의 첫 데이터그램을 받아 해당 주소로 소켓을 연결하고,
 * 다음 스트림을 위한 리스닝 소켓을 새로 준비.
 *
 * \param test 현재 iperf 테스트 객체
 * \return 성공 시 스트림 소켓 디스크립터, 실패 시 음수 (-1 또는 내부 검사 오류 코드)
 */
int iperf_udp_accept(struct iperf_test *test);

/**
 * \brief 서버 측 UDP 리스닝 소켓 생성
 * \param test 현재 iperf 테스트 객체
 * \return 리스닝 소켓 디스크립터, 실패 시 -1
 */
int iperf_udp_listen(struct iperf_test *test);

/**
 * \brief 클라이언트 측에서 서버로 UDP 스트림 연결
 *
 * 연결 요청용 데이터그램을 보내고 서버 응답을 받아야 연결 완료.
 *
 * \param test 현재 iperf 테스트 객체
 * \return 연결된 소켓 디스크립터, 실패 시 음수 (-1 또는 내부 검사 오류 코드)
 */
int iperf_udp_connect(struct iperf_test *test);

/**
 * \brief UDP 테스트 초기화
 * \param test 현재 iperf 테스트 객체
 * \return 성공 시 0
 */
int iperf_udp_init(struct iperf_test *test);

#endif
