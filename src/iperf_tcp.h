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
 * \file   iperf_tcp.h
 * \brief  iperf3 TCP 테스트용 함수 선언
 * \author 이준상
 * \date   2026-11-09
 *
 * TCP 데이터 스트림의 연결 수립과 데이터 송수신 담당.
 *   - 서버 측
 *     -# iperf_tcp_listen() : 데이터 스트림용 리스닝 소켓 준비
 *     -# iperf_tcp_accept() : 클라이언트 연결 수락
 *   - 클라이언트 측
 *     -# iperf_tcp_connect() : 서버에 데이터 스트림 연결
 *   - 공통
 *     -# iperf_tcp_send() / iperf_tcp_recv() : 데이터 송수신
 */
#ifndef        IPERF_TCP_H
#define        IPERF_TCP_H

/**
 * \brief 리스닝 소켓에서 새 TCP 데이터 연결 수락
 *
 * 연결 후 클라이언트가 보낸 \b 쿠키를 확인해
 * 같은 테스트에 속한 연결인지 검증.
 *
 * \param test 현재 iperf 테스트 객체
 * \return 성공 시 수락된 소켓 디스크립터, 실패 시 -1
 */
int iperf_tcp_accept(struct iperf_test *test);

/**
 * \brief TCP 스트림에서 데이터 수신
 * \param sp 데이터를 받을 스트림
 * \return 수신한 바이트 수, 오류 시 음수
 */
int iperf_tcp_recv(struct iperf_stream *sp);

/**
 * \brief TCP 스트림으로 데이터 전송
 *
 * \c -Z (zerocopy) 옵션 사용 시 sendfile 방식으로 전송.
 *
 * \param sp 데이터를 보낼 스트림
 * \return 전송한 바이트 수, 오류 시 음수
 */
int iperf_tcp_send(struct iperf_stream *sp) /* __attribute__((hot)) */;

/**
 * \brief 서버 측 TCP 데이터 스트림용 리스닝 소켓 준비
 *
 * 윈도우 크기, MSS 등 테스트 옵션에 맞춰 필요하면 소켓을 다시 생성.
 *
 * \param test 현재 iperf 테스트 객체
 * \return 리스닝 소켓 디스크립터, 실패 시 -1
 */
int iperf_tcp_listen(struct iperf_test *test);

/**
 * \brief 클라이언트 측에서 서버로 TCP 데이터 스트림 연결
 *
 * 소켓 옵션(윈도우 크기, MSS, No-delay, 혼잡 제어 등) 설정 후
 * 연결하고 테스트 쿠키 전송.
 *
 * \param test 현재 iperf 테스트 객체
 * \return 연결된 소켓 디스크립터, 실패 시 -1
 */
int iperf_tcp_connect(struct iperf_test *test);

#endif
