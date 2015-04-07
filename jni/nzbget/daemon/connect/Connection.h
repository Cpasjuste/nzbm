/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2004 Sven Henkel <sidddy@users.sourceforge.net>
 *  Copyright (C) 2007-2015 Andrey Prygunkov <hugbug@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Revision: 1250 $
 * $Date: 2015-03-31 21:52:57 +0200 (mar. 31 mars 2015) $
 *
 */


#ifndef CONNECTION_H
#define CONNECTION_H

#ifndef HAVE_GETADDRINFO
#ifndef HAVE_GETHOSTBYNAME_R
#include "Thread.h"
#endif
#endif
#ifndef DISABLE_TLS
#include "TLS.h"
#endif

class Connection
{
public:
	enum EStatus
	{
		csConnected,
		csDisconnected,
		csListening,
		csCancelled
	};

protected:
	char*				m_szHost;
	int					m_iPort;
	SOCKET				m_iSocket;
	bool				m_bTLS;
	char*				m_szCipher;
	char*				m_szReadBuf;
	int					m_iBufAvail;
	char*				m_szBufPtr;
	EStatus				m_eStatus;
	int					m_iTimeout;
	bool				m_bSuppressErrors;
	char				m_szRemoteAddr[20];
	int					m_iTotalBytesRead;
	bool				m_bBroken;

	struct SockAddr
	{
		int				ai_family;
		int				ai_socktype;
		int				ai_protocol;
		bool			operator==(const SockAddr& rhs) const
							{ return memcmp(this, &rhs, sizeof(SockAddr)) == 0; }
	};

#ifndef DISABLE_TLS
	class ConTLSSocket: public TLSSocket
	{
	private:
		Connection*		m_pOwner;
	protected:
		virtual void	PrintError(const char* szErrMsg) { m_pOwner->PrintError(szErrMsg); }
	public:
						ConTLSSocket(SOCKET iSocket, bool bIsClient, const char* szCertFile,
							const char* szKeyFile, const char* szCipher, Connection* pOwner):
							TLSSocket(iSocket, bIsClient, szCertFile, szKeyFile, szCipher), m_pOwner(pOwner) {}
	};

	ConTLSSocket*		m_pTLSSocket;
	bool				m_bTLSError;
#endif
#ifndef HAVE_GETADDRINFO
#ifndef HAVE_GETHOSTBYNAME_R
	static Mutex*		m_pMutexGetHostByName;
#endif
#endif

						Connection(SOCKET iSocket, bool bTLS);
	void				ReportError(const char* szMsgPrefix, const char* szMsgArg, bool PrintErrCode, int herrno);
	virtual void		PrintError(const char* szErrMsg);
	bool				DoConnect();
	bool				DoDisconnect();
	bool				InitSocketOpts();
	bool				ConnectWithTimeout(void* address, int address_len);
#ifndef HAVE_GETADDRINFO
	unsigned int		ResolveHostAddr(const char* szHost);
#endif
#ifndef DISABLE_TLS
	int					recv(SOCKET s, char* buf, int len, int flags);
	int					send(SOCKET s, const char* buf, int len, int flags);
	void				CloseTLS();
#endif

public:
						Connection(const char* szHost, int iPort, bool bTLS);
	virtual 			~Connection();
	static void			Init();
	static void			Final();
	virtual bool 		Connect();
	virtual bool		Disconnect();
	bool				Bind();
	bool				Send(const char* pBuffer, int iSize);
	bool				Recv(char* pBuffer, int iSize);
	int					TryRecv(char* pBuffer, int iSize);
	char*				ReadLine(char* pBuffer, int iSize, int* pBytesRead);
	void				ReadBuffer(char** pBuffer, int *iBufLen);
	int					WriteLine(const char* pBuffer);
	Connection*			Accept();
	void				Cancel();
	const char*			GetHost() { return m_szHost; }
	int					GetPort() { return m_iPort; }
	bool				GetTLS() { return m_bTLS; }
	const char*			GetCipher() { return m_szCipher; }
	void				SetCipher(const char* szCipher);
	void				SetTimeout(int iTimeout) { m_iTimeout = iTimeout; }
	EStatus				GetStatus() { return m_eStatus; }
	void				SetSuppressErrors(bool bSuppressErrors);
	bool				GetSuppressErrors() { return m_bSuppressErrors; }
	const char*			GetRemoteAddr();
#ifndef DISABLE_TLS
	bool				StartTLS(bool bIsClient, const char* szCertFile, const char* szKeyFile);
#endif
	int					FetchTotalBytesRead();
};

#endif
