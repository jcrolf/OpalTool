/*******************************************************************************
 * Copyright (c) 2015, The Johns Hopkins University Applied Physics Laboratory *
 * LLC All rights reserved.                                                    *
 *                                                                             *
 * Redistribution and use in source and binary forms, with or without modifica-*
 * tion, are permitted provided that the following conditions are met:         *
 *                                                                             *
 * 1. Redistributions of source code must retain the above copyright notice,   *
 *    this list of conditions and the following disclaimer.                    *
 * 2. Redistributions in binary form must reproduce the above copyright notice,*
 *    this list of conditions and the following disclaimer in the documentation*
 *    and/or other materials provided with the distribution.                   *
 * 3. Neither the name of the copyright holder nor the names of its contri-    *
 *    butors may be used to endorse or promote products derived from this      *
 *    software without specific prior written permission.                      *
 *                                                                             *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" *
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE   *
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE  *
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE   *
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CON-    *
 * SEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE*
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) *
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT  *
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY   *
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH *
 * DAMAGE.                                                                     *
 *                                                                             *
 ******************************************************************************/


// TakeControlTool.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <string>
#include <sys/stat.h>

#include <errno.h>

#include <openssl/rsa.h>
#include <openssl/ossl_typ.h>
#include <openssl/ms/applink.c>

#include "OpalToolC_API.h"


//#define DEBUG_MODE
#ifdef DEBUG_MODE
#define DEBUG_OUT(a) std::cerr << "DEBUG: " << a
#define BOOL_TEXT(a) ((a) ? "true" : "false")
#else
#define DEBUG_OUT(a)
#endif

static std::string versionString="2.0.1";

bool CloseFile(FILE *fileID,const char* fname)
{
	bool ok=true;
	// just handle the close and any non-zero rc
	if (fileID!=NULL) {
		if (fname==NULL) fname="unknown";

		if (fclose(fileID)!=0)
			std::cerr << "ERROR: Unable to properly close '" << fname << "': " << strerror_s(strerrorBuf,errno) << "\n";
	}

	return ok;
}

class args {
public:

	args() { _commandName="Unknown"; }

	void version() { std::cout << "Version = " << versionString << "\n"; }

	void help()
	{
		std::cout << "Usage: " << _commandName << " <options>\n";
		std::cout << "\n";
		std::cout << "<options> ::= <order dependent options>\n";
		std::cout << "              <order independent options>\n";
		std::cout << "              --version\n";
		std::cout << "\n";
		std::cout << "<order dependent options>\n";
		std::cout << "          ::= <decimal number> <path to pubkey> <path to encrypted PW>\n";
		std::cout << "\n";
		std::cout << "<order independent options>\n";
		std::cout << "          ::= { --DRIVE <decimal number> | --PUBKEY <path to pubkey> | --PW <path to encrypted PW> }*\n";
		std::cout << "\n";
	}

	bool parseArgs(int argc, char* argv[])
	{
		bool ok=true;

		if (argv[0]!=nullptr) {
			char *slash=strrchr(argv[0],'\\');
			if (slash)
				 _commandName=slash+1;
			else _commandName=argv[0];
		}

		for (int i=1;i<argc; i++)
		{
			bool optionOcclusion;

			if (strncmp(argv[i],"--",2)==0) {
				char *argType=argv[i]+2;
				if (strcmp(argType,"DRIVE")==0)
				{
					if (optionOcclusion=(_driveNumber!=""))
						std::cout << "WARNING: override of previous declaration of the DRIVE (" << _driveNumber << ") with: ";
					_driveNumber=(++i>=argc) ? "" : argv[i];// This gets passed directly to the opaltoolc command line
					if (optionOcclusion) { std::cout << _driveNumber; }
				}
				else if (strcmp(argType,"PUBKEY")==0)
				{
					if (optionOcclusion=(_pubKeyPath!=""))
						std::cout << "WARNING: override of previous declaration of the PUBKEY (" << _pubKeyPath << ") with: ";
					_pubKeyPath=(++i>=argc) ? "" : argv[i];
					if (optionOcclusion) { std::cout << _pubKeyPath << "\n"; }
				}
				else if (strcmp(argType,"PW")==0)
				{
					if (optionOcclusion=(_encryptedPWPath!=""))
						std::cout << "WARNING: override of previous declaration of the PW (" << _encryptedPWPath << ") with: ";
					_encryptedPWPath=(++i>=argc) ? "" : argv[i];
					if (optionOcclusion) { std::cout << _encryptedPWPath << "\n"; }
				}
				else if (strcmp(argType,"help")==0)
				{
					help();
					exit(1);
				}
				else if (strcmp(argType,"version")==0)
				{
					version();
					exit(0);
				}
			}
			else {
				if (_driveNumber=="") {_driveNumber=argv[i]; }
				else if (_pubKeyPath=="") {_pubKeyPath=argv[i]; }
				else if (_encryptedPWPath=="") {_encryptedPWPath=argv[i]; }
			}
		}

		ok=argsAudit();
		ok&=filesAudit();

		return ok;
	}
	
	const std::string &driveNumber() { return _driveNumber; }
	const std::string &pubKeyPath() { return _pubKeyPath; }
	const std::string &encryptedPWPath() { return _encryptedPWPath; }

private:

	bool fileAudit(const char *fileName, const char* fileType, bool write=false)
	{
		bool ok=false;
		struct stat b;

		if (stat(fileName,&b)==0) {
			if (write) {
				std::cerr << "\n\nERROR: The " << fileType << " file '" << fileName << "' already exists, take control aborted.\n";
				std::cerr << "ERROR: Ensure the drive is not already set to the password encrypted in this file. \n";
				std::cerr << "       If not, then remove the file and try again.\n\n";
			}
			else {
				FILE *test = nullptr;
				fopen_s(&test,fileName,"rb");
				if (test==NULL) {
					std::cerr << "ERROR: Unable to read " << fileName << ": " << strerror_s(strerrorBuf,errno) << "\n";
				}
				else { 
					ok=CloseFile(test,fileName); // true unless the close itself fails
				}
			}
		}
		else if (!write) { std::cerr << "ERROR: The " << fileType << ") file '" << fileName << "' can't be located\n"; }
		else { // we make sure we can open the file for writing ...
			FILE *test = nullptr;
			fopen_s(&test,fileName,"wb");
			if (test==NULL) {
				std::cerr << "ERROR: Unable to create/open for writing " << fileName << ": " << strerror_s(strerrorBuf,errno) << "\n";
			}
			else {  
				ok = CloseFile(test,fileName); // true unless the close itself fails ...
			}
		}

		DEBUG_OUT("fileaudit of " << fileName << " (" << fileType << ") results in " << BOOL_TEXT(ok) << "\n");
		return ok;
	}

	bool filesAudit()
	{
		bool ok=fileAudit(_pubKeyPath.c_str(),"pubkey");
		ok &= fileAudit(_encryptedPWPath.c_str(),"encrypted password",true); 
		return ok;
	}

	bool argsAudit()
	{
		bool ok=false;

		if (_driveNumber=="")			{ std::cerr << "ERROR: A drive number must be specified.\n"; }
		else if (_pubKeyPath=="")		{ std::cerr << "ERROR: A path to a file containing the public key must be specified.\n"; }
		else if (_encryptedPWPath=="")	{ std::cerr << "ERROR: A path to a file containing the encryted PW must be specified.\n"; }
		else							{ ok=true; }

		DEBUG_OUT("fileaudit of arguments results in " << BOOL_TEXT(ok) << "\n");
		return ok;
	}

	std::string _driveNumber;
	std::string _pubKeyPath;
	std::string _encryptedPWPath;
	std::string _commandName;
};



bool weAreAdmin(void)
{
	bool isAdmin=false;
    TOKEN_ELEVATION_TYPE    tet;
    TOKEN_ELEVATION         te;
    HANDLE                  hToken;
    DWORD                   dwReturnLength;

    /* Open a handle to our token. */
    if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {

		/* Get elevation type information. */
		GetTokenInformation(hToken, TokenElevationType, &tet, sizeof(tet), &dwReturnLength);
		GetTokenInformation(hToken, TokenElevation, &te, sizeof(te), &dwReturnLength);

		/* Close the handle to the token. */
		CloseHandle(hToken);

		/* The following can only happen if UAC is enabled and the user is an admin. */
		if(tet == TokenElevationTypeFull) {
			isAdmin=true;
		}
		/* If we get here, either UAC is enabled and we're a normal user, or UAC is disabled. */
		else if (te.TokenIsElevated) {
            isAdmin=true;
	    }  
    }

	return isAdmin;
}


#include <stdio.h>


// We do a dynamic load of the openssl windows library, these define all the module names and their types
// 
typedef RSA* (*PEM_read_RSA_PUBKEY_ptr_type)(FILE*,void*,void*,void*);
PEM_read_RSA_PUBKEY_ptr_type PEM_read_RSA_PUBKEY_ptr;
std::string PEM_read_RSA_PUBKEY_str="PEM_read_RSA_PUBKEY";
typedef RSA* (*PEM_read_RSAPrivateKey_ptr_type)(FILE*,void*,void*,void*);
PEM_read_RSAPrivateKey_ptr_type PEM_read_RSAPrivateKey_ptr;
std::string PEM_read_RSAPrivateKey_str="PEM_read_RSAPrivateKey";
typedef int (*RSA_private_decrypt_ptr_type)(int,const unsigned char*,unsigned char*,RSA*,int);
RSA_private_decrypt_ptr_type RSA_private_decrypt_ptr;
std::string RSA_private_decrypt_str="RSA_private_decrypt";
typedef int (*RSA_public_encrypt_ptr_type)(int,const unsigned char*,unsigned char*,RSA*,int);
RSA_public_encrypt_ptr_type RSA_public_encrypt_ptr;
std::string RSA_public_encrypt_str="RSA_public_encrypt";
typedef int (*RSA_size_ptr_type)(RSA*);
RSA_size_ptr_type RSA_size_ptr;
std::string RSA_size_str="RSA_size";


typedef void (*ERR_load_crypto_strings_ptr_type)();
ERR_load_crypto_strings_ptr_type ERR_load_crypto_strings_ptr;
std::string ERR_load_crypto_strings_str="ERR_load_crypto_strings";
typedef char* (*ERR_error_string_ptr_type)(unsigned long,char*);
ERR_error_string_ptr_type ERR_error_string_ptr;
std::string ERR_error_string_str="ERR_error_string";
typedef unsigned long (*ERR_get_error_ptr_type)();
ERR_get_error_ptr_type ERR_get_error_ptr;
std::string ERR_get_error_str="ERR_get_error";


bool libSetup(HMODULE &encryptLib)
{	
	bool ok=false;
		
	encryptLib = LoadLibraryA("libeay32.dll");

	if (encryptLib!=NULL)
	{
		if ((PEM_read_RSA_PUBKEY_ptr=(PEM_read_RSA_PUBKEY_ptr_type)GetProcAddress(encryptLib,PEM_read_RSA_PUBKEY_str.c_str()))==NULL)
			std::cerr << "Failed to find " << PEM_read_RSA_PUBKEY_str << ": \n";
		if ((PEM_read_RSAPrivateKey_ptr=(PEM_read_RSAPrivateKey_ptr_type)GetProcAddress(encryptLib,PEM_read_RSAPrivateKey_str.c_str()))==NULL)
			std::cerr << "Failed to find " << PEM_read_RSAPrivateKey_str << ": \n";
		if ((ERR_load_crypto_strings_ptr=(ERR_load_crypto_strings_ptr_type)GetProcAddress(encryptLib,ERR_load_crypto_strings_str.c_str()))==NULL)
			std::cerr << "Failed to find " << ERR_load_crypto_strings_str << ": \n";
		if ((ERR_get_error_ptr=(ERR_get_error_ptr_type)GetProcAddress(encryptLib,ERR_get_error_str.c_str()))==NULL)
			std::cerr << "Failed to find " << ERR_get_error_str << ": \n";
		if ((RSA_private_decrypt_ptr=(RSA_private_decrypt_ptr_type)GetProcAddress(encryptLib,RSA_private_decrypt_str.c_str()))==NULL)
			std::cerr << "Failed to find " << RSA_private_decrypt_str << ": \n";
		if ((RSA_public_encrypt_ptr=(RSA_public_encrypt_ptr_type)GetProcAddress(encryptLib,RSA_public_encrypt_str.c_str()))==NULL)
			std::cerr << "Failed to find " << RSA_public_encrypt_str << ": \n";
		if ((RSA_size_ptr=(RSA_size_ptr_type)GetProcAddress(encryptLib,RSA_size_str.c_str()))==NULL)
			std::cerr << "Failed to find " << RSA_size_str << ": \n";
		else ok=true;
	}
	else 
	{
		DWORD errorValue=GetLastError();
		std::cerr << "The library failed to load, the error is " << (void*)errorValue << ": " << strerror_s(strerrorBuf,errorValue) << "\n";
	}

	return ok;
}
	  
bool getRandomPassword(args &a,string &pwBuffer)
{
	bool ok=false;

	BYTE RandomBytes[32];

    MemInit(NULL);
		 
	string Drive="\\\\.\\PhysicalDrive" + a.driveNumber();
	DEBUG_OUT("Drive is: " << Drive << "\n");
	if (GetRandom_C(Drive,RandomBytes)==0) {
		char buf[3];
		for (int i = 0; i<32 ; i++) {
			sprintf_s(buf,"%02x",RandomBytes[i]);
			pwBuffer += buf;
		}
		DEBUG_OUT("Our random bytes are: " << pwBuffer << "\n");
		ok=true;
	}

	MemCleanup();

	return ok;
}

bool encyptedFileAudit(args &a, const unsigned char* encryptedBytes, const int size)
{
	bool ok=false;
	struct stat b;

	if (stat(a.encryptedPWPath().c_str(),&b)==0) {
		DEBUG_OUT("created PW file: stat success ...\n");
		if (b.st_size==size) {
			DEBUG_OUT("created PW file: size match ...\n");
			FILE *test = nullptr;
			fopen_s(&test,a.encryptedPWPath().c_str(),"rb");
			if (test!=NULL) {
				DEBUG_OUT("created PW file: open success ...\n");
				unsigned char* tempBuf = new unsigned char[size];

				if (tempBuf==NULL) {
					std::cerr << "ERROR: Unable to allocate memory " << __FILE__ << "!\n";
				}
				else if (fread(tempBuf,1,size,test)!=size) {
					std::cerr << "ERROR: Unable to validate the enctypted buffer contents against the file contents: " << strerror_s(strerrorBuf,errno) << "\n";
				}
				else if (memcmp(tempBuf,encryptedBytes,size)!=0) {
					std::cerr << "ERROR: The enctyped file contents do not match the internal encrypted buffer\n";
				}
				else { ok=true; DEBUG_OUT("created PW file: content match!\n");}

				delete [] tempBuf;

				ok=CloseFile(test,a.encryptedPWPath().c_str());
			}
			else {
				std::cerr << "ERROR: Unable to open " << a.encryptedPWPath() << ": " << strerror_s(strerrorBuf,errno) << "\n";
			}
		}
		else {
			std::cerr << "ERROR: size mismatch: created encrypted password file and the actual encrypted buffer\n";
		}
	}
	else {
		std::cerr << "ERROR: Unable to stat " << a.encryptedPWPath() << ": " << strerror_s(strerrorBuf,errno) << "\n";
	}

	return ok;
}

bool encryptRandomPassword(args &a, string &pwBuffer)
{
	bool ok=false;
	char   errorBuffer[1024];
	FILE  *pemPubkey=NULL;

	if (pwBuffer.length()==64) {
		fopen_s(&pemPubkey,a.pubKeyPath().c_str(),"rb");

		if (pemPubkey!=NULL) {
			RSA   *rsaptr=PEM_read_RSA_PUBKEY_ptr(pemPubkey,NULL,NULL,NULL);
			unsigned char* encrypted = new unsigned char[RSA_size_ptr(rsaptr)];
			int            encrypted_len;

			if (encrypted) {
				if (rsaptr!=NULL) {
					if ((encrypted_len=RSA_public_encrypt_ptr(pwBuffer.length(),(const unsigned char*)pwBuffer.c_str(),
														  encrypted,rsaptr,RSA_PKCS1_PADDING))!=-1) 
					{
						FILE *encFile=NULL;
						fopen_s(&encFile,a.encryptedPWPath().c_str(),"wb");
						if (encFile!=NULL) {
							if (fwrite(encrypted,1,encrypted_len,encFile)==encrypted_len) { ok=true; }
							else { std::cerr << "Unable to write the encrypted password file '" << a.encryptedPWPath() << "': " << strerror_s(strerrorBuf,errno) << "\n"; }
						
							if (ok &= CloseFile(encFile,a.encryptedPWPath().c_str())) {
								// One final audit - very important this file was created correctly ...
								ok = encyptedFileAudit(a, encrypted, encrypted_len);
							}		
						}
						else { std::cerr << "Unable to create the encrypted password file '" << a.encryptedPWPath() << "': " << strerror_s(strerrorBuf,errno) << "\n"; }
					}
					else {
						ERR_load_crypto_strings_ptr();
						ERR_error_string_ptr(ERR_get_error_ptr(),errorBuffer);
						std::cerr << "RSA error encryting the random bytes with the public key = " << errorBuffer << "\n";
					}
				}
				else {
					ERR_load_crypto_strings_ptr();
					ERR_error_string_ptr(ERR_get_error_ptr(),errorBuffer);
					std::cerr << "RSA error accessing the public key = " << errorBuffer << "\n";
				}
				delete [] encrypted;
			}
			else { std::cerr << "FATAL: Failure to allocate the encryption buffer\n"; }

			ok &= CloseFile(pemPubkey,a.pubKeyPath().c_str());
		}
		else { std::cerr << "ERROR: Unable to open " << a.pubKeyPath() << ": " << strerror_s(strerrorBuf,errno) << "\n"; }
	}
	else { std::cerr << "ERROR: Invalid random password length " << pwBuffer.length() << "\n"; }

	return ok;
}

bool changeSIDpassword(args &a, const std::string &pwBuffer)
{
	bool ok=false;
	string pwHex="0x" + pwBuffer;
	const char *otcArgs[]={"",
		             "--DRIVE",a.driveNumber().c_str(),
		             "--changePassword",
					 "--UID","SID",
					 "--PW","MSID",
					 "--TUID","SID",
					 "--TPW",pwHex.c_str() };

	DEBUG_OUT("incoming password = 0x" << pwBuffer << "\n");
	Arguments opaltoolArgs((int)12,otcArgs);
	DEBUG_OUT("OTCARGS: Drive = " << opaltoolArgs.drive() << "\n");
	DEBUG_OUT("OTCARGS: Action = " << opaltoolArgs.actionName() << "\n");
	DEBUG_OUT("OTCARGS: UID = " << opaltoolArgs.uid() << "\n");
	DEBUG_OUT("OTCARGS: password = " << opaltoolArgs.password().asString() << "\n");
	DEBUG_OUT("OTCARGS: target UID = " << opaltoolArgs.targetuid() << "\n");
	DEBUG_OUT("OTCARGS: target password = " << opaltoolArgs.targetPassword().asString() << "\n");


	if (changePassword_C(opaltoolArgs)==0) {
		std::cerr << "SUCCESS: the SID for the specified OPAL drive has been reset to a random password.\n";
		std::cerr << "         the encypted password can be found in the file '" << a.encryptedPWPath() << "'\n";
		std::cerr << "         use the private key associated with the public key you used (" << a.pubKeyPath() << ") to\n";
		std::cerr << "         determine what that password is:\n";
		std::cerr << "\n";
		std::cerr << "         openssl rsautl -decrypt -in " << a.encryptedPWPath() << " -inkey <privateKeyFile> -out " 
			      << a.encryptedPWPath() << ".dec\n\n";
		ok=true;
	}

	return ok;
}

int main(int argc, char* argv[])
{
	int rc=1;

	if (weAreAdmin())
	{
		args a;
		if (a.parseArgs(argc,argv))
		{		
			DEBUG_OUT("we parsed the args\n");
			HMODULE encryptLib;

		    if (libSetup(encryptLib)) {
				std::string pwBuffer;

				debugTrace T;
				T.setDebugTrace(debugTrace::TRACE_OFF);

				if (getRandomPassword(a,pwBuffer)) {
					DEBUG_OUT("we have the random password: '" << pwBuffer << "'\n");
					if (encryptRandomPassword(a,pwBuffer)) {
						DEBUG_OUT("we encrypted the password\n");	
						if (changeSIDpassword(a,pwBuffer)) {
							DEBUG_OUT("SID password changed from MSID to 0x" << pwBuffer << "\n");
							rc=0;
						}
						else { std::cerr << "ERROR: Unable to change the SID password.\n"; }
					}
					else { std::cerr << "ERROR: Unable to encrypt the random key.\n"; }
				}
				else { std::cerr << "ERROR: Unable to get a random key from the specified drive.\n"; }
			}
			else { std::cerr << "ERROR: Unable to establish the openssl interface.\n"; }
		}
		else { std::cerr << "ERROR: Failure parsing the arguments.\n"; }
	}
	else { std::cerr << "ERROR: This function can only be executed from an command prompt with admin priviledges.\n"; }

	return rc;
}
