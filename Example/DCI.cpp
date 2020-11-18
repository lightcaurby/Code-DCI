// DCI.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <assert.h>

#include <string>
#include <iostream>
#include <sstream>
#include <list>
using namespace std;


const unsigned MAX_BUFFER_SIZE = 256;

////////////////////////////////////////////////


/////////////////////////////////////////////////

void endTransaction( void ) {
	std::cout << "::endTransaction(void)" << std::endl;
}

void beginTransaction( void ) {
	std::cout << "::beginTransaction(void)" << std::endl;
}

void displayScreen( bool b ) {
	std::cout << ( b ? "OK" : "NOT OK" ) << std::endl;
}

////////////////////////////////////////////////

class MyException
{
public:
};

class InsufficientFunds :
	public MyException {
public:
	InsufficientFunds( void ) {};
};


/////////////////////////////////////////////////


class Currency {
public:
	virtual Currency &operator+=( const Currency& amount ) {
		amount_ += amount.amountInEuro();
		return *this;
	}
	virtual Currency &operator-=( const Currency& amount ) {
		amount_ -= amount.amountInEuro();
		return *this;
	}
	virtual Currency &operator=( const Currency& amount ) {
		amount_ = amount.amountInEuro();
		return *this;
	}
	Currency( const Currency& amount ) {
		amount_ = amount.amountInEuro();
	}
	Currency( void ) : amount_( 0 ) {};
	Currency( double amount ) : amount_( amount ) {};
	virtual ~Currency() { }
	virtual double amountInEuro( void ) const { return amount_; }

	friend std::ostream &operator<<( std::ostream &stream, Currency &currency )
	{
		stream << currency.amountInEuro();
		return stream;
	}
	bool operator<( const Currency &other ) { return amountInEuro() < other.amountInEuro(); }
	bool operator==( const Currency &other ) { return amountInEuro() == other.amountInEuro(); }
	bool operator>( const Currency &other ) { return amountInEuro() > other.amountInEuro(); }
	bool operator<=( const Currency &other ) { return amountInEuro() <= other.amountInEuro(); }
	bool operator>=( const Currency &other ) { return amountInEuro() >= other.amountInEuro(); }

private:
	double amount_;
};

/////////////////////////////////////////////////

class IAccountDomainRole {
public:
	virtual string accountID( void ) const = 0;

	virtual Currency availableBalance( void ) = 0;
	virtual void decreaseBalance( Currency ) = 0;
	virtual void increaseBalance( Currency ) = 0;
	virtual void updateLog( string, Currency ) = 0;

public:
	virtual ~IAccountDomainRole() {}
};

class Account :
	public IAccountDomainRole
{
public:
	virtual string accountID( void ) const { return ""; };

public:
	virtual ~Account() {}
};

/////////////////////////////////////////////////

class IMoneySinkRole;
class Creditor;
class PayBillsContext;
class TransferMoneyContext;


class IMoneySourceRole
{
public:
	virtual Currency availableBalance() = 0;
	virtual void updateLog( string, Currency amount ) = 0;

	virtual void transferTo( Currency amount, IMoneySinkRole *destinationAccount ) = 0;
	virtual void decreaseBalance( Currency amount ) = 0;
	virtual void payBills( std::list< Creditor* >& creditors ) = 0;

public:
	virtual ~IMoneySourceRole() {}
};

////////////////////////////////////////////////

class IMoneySourceRole;

class IMoneySinkRole
{
public:
	virtual Currency availableBalance() = 0;
	virtual void updateLog( string, Currency amount ) = 0;

	virtual void increaseBalance( Currency amount ) = 0;

public:
	virtual ~IMoneySinkRole() {}
};

/////////////////////////////////////////////////

class Context {
public:
	Context( void );
	virtual ~Context();
public:
	static Context *currentContext_;
private:
	Context *parentContext_;
};

Context::Context( void ) {
	parentContext_ = currentContext_;
	currentContext_ = this;
}

Context::~Context() {
	currentContext_ = parentContext_;
}

Context *Context::currentContext_ = NULL;

////////////////////////////////////////////////

//***#define SELF static_cast<ConcreteDerived*>(this)

/***
#define CONTEXT (((dynamic_cast<TransferMoneyContext*>(Context::currentContext_)?	\
			dynamic_cast<TransferMoneyContext*>(Context::currentContext_):	\
			(throw("dynamic cast failed"), static_cast<TransferMoneyContext*>(NULL))	\
			)))
***/

// #define RECIPIENT ((static_cast<TransferMoneyContext*>(Context::currentContext_)->destinationAccount()))


/***
#define CREDITORS ((static_cast<PayBillsContext*>(Context::currentContext_)->creditors()))
***/


////////////////////////////////////////////////

template < class ConcreteDerived >
class MoneySourceRole :
	public IMoneySourceRole
{

public:
	virtual ConcreteDerived* SELF() = 0;
	IMoneySourceRole* ROLE() { return static_cast< IMoneySourceRole* >( this ); }

public:
	// Role behaviors
	virtual void payBills( std::list< Creditor* >& creditors ) {
		// While object contexts are changing, we don't want to
		// have an open iterator on an external object. Make a
		// local copy.
		std::list< Creditor* >::iterator iter = creditors.begin();
		for(; iter != creditors.end(); iter++) {
			try {
				// Note that here we invoke another Use Case
				TransferMoneyContext transferTheFunds(
					( *iter )->amountOwed(),
					ROLE(),
					( *iter )->account() );
				transferTheFunds.doit();
			}
			catch(InsufficientFunds) {
				throw;
			}
		}
	}

	virtual void transferTo( Currency amount, IMoneySinkRole *destinationAccount ) {
		// This code is reviewable and
		// meaningfully testable with stubs!
		beginTransaction();
		if(ROLE()->availableBalance() < amount) {
			endTransaction();
			throw InsufficientFunds();
		}
		else {
			ROLE()->decreaseBalance( amount );
			destinationAccount->increaseBalance( amount );
			ROLE()->updateLog( "Transfer Out", amount );
			destinationAccount->updateLog( "Transfer In", amount );
		}
		endTransaction();
		displayScreen( true );
	}

	Currency availableBalance()
	{
		return SELF()->availableBalance();
	};

	void decreaseBalance( Currency amount )
	{
		SELF()->decreaseBalance( amount );
	}

	void updateLog( string s, Currency amount )
	{
		SELF()->updateLog( s, amount );
	}

public:
	MoneySourceRole( void ) { }

	virtual ~MoneySourceRole() {}
};

////////////////////////////////////////////////

template <class ConcreteDerived>
class MoneySinkRole :
	public IMoneySinkRole
{
public:
	virtual ConcreteDerived* SELF() = 0;
	IMoneySinkRole* ROLE() { return static_cast< IMoneySinkRole* >( this ); }

public:
	void transferFrom( Currency amount ) {
		ROLE()->increaseBalance( amount );
		ROLE()->updateLog( "Transfer in", amount );
	}

	Currency availableBalance()
	{
		return SELF()->availableBalance();
	};

	void increaseBalance( Currency amount )
	{
		SELF()->increaseBalance( amount );
	}

	void updateLog( string s, Currency amount )
	{
		SELF()->updateLog( s, amount );
	}

public:
	MoneySinkRole( void ) {
	}

	virtual ~MoneySinkRole() {}

};

/////////////////////////////////////////////////

class XferMoneyContext;
class PayBillsContext;

class CheckingAccount :
	public Account
{
public:
	CheckingAccount( void ) {};

	virtual ~CheckingAccount( void ) {};

public:
	// These functions can be virtual if there are
	// specializations of CheckingAccount
	virtual Currency availableBalance( void );
	virtual void decreaseBalance( Currency );
	virtual void increaseBalance( Currency );
	virtual void updateLog( string, Currency );
private:
	Currency availableBalance_;
};

void CheckingAccount::updateLog( string logMessage,
	Currency amountForTransaction ) {
	assert( this != NULL );
	assert( logMessage.size() > 0 );
	assert( logMessage.size() < MAX_BUFFER_SIZE );
	std::cout << "account: " << accountID() << " CheckingAccount::updateLog(\"" << logMessage << "\", " << amountForTransaction << ")"
		<< std::endl;
	assert( true );
}

void CheckingAccount::increaseBalance( Currency c ) {
	assert( this != NULL );
	std::cout << "CheckingAccount::increaseBalance(" << c << ")" << std::endl;
	availableBalance_ += c;
	assert( true );
}

/////////////////////////////////////////////////

class XferMoneyContext;
class PayBillsContext;

class InvestmentAccount :
	public Account
{
public:
	InvestmentAccount( void );
	virtual ~InvestmentAccount( void ) {};

	virtual Currency availableBalance( void );
	virtual void decreaseBalance( Currency );
	virtual void increaseBalance( Currency );
	virtual void updateLog( string, Currency );
private:
	Currency availableBalance_;
};

/////////////////////////////////////////////////

InvestmentAccount::InvestmentAccount( void ) :
	availableBalance_( Currency( 0.00 ) )
{
}

Currency InvestmentAccount::availableBalance( void ) {
	std::cout << "InvestmentAccount::availableBalance returns "
		<< availableBalance_ << std::endl;
	return availableBalance_;
}

void
InvestmentAccount::increaseBalance( Currency c ) {
	std::cout << "InvestmentAccount::increaseBalance(" << c << ")" << std::endl;
	availableBalance_ += c;
}

void
InvestmentAccount::decreaseBalance( Currency c ) {
	std::cout << "InvestmentAccount::decreaseBalance(" << c << ")" << std::endl;
	availableBalance_ -= c;
}

void
InvestmentAccount::updateLog( string s, Currency c ) {
	std::cout << "account: " << accountID() << " InvestmentAccount::updateLog(\"" << s << "\", " << c << ")" << std::endl;
}

/////////////////////////////////////////////////

class XferMoneyContext;
class PayBillsContext;

class SavingsAccount :
	public Account
{
public:
	SavingsAccount( void );
	virtual ~SavingsAccount( void ) {};

public:
	// These functions can be virtual if there are
	// specializations of SavingsAccount
	virtual Currency availableBalance( void );
	virtual void decreaseBalance( Currency );
	virtual void increaseBalance( Currency );
	virtual void updateLog( string, Currency );
private:
	Currency availableBalance_;
};

/////////////////////////////////////////////////

SavingsAccount::SavingsAccount( void ) :
	availableBalance_( Currency( 0.00 ) )
{
	assert( this != NULL );

	// no application code

	assert( availableBalance_ == Currency( 0.0 ) );
	assert( true );
}

Currency SavingsAccount::availableBalance( void ) {
	assert( this != NULL );
	std::cout << "SavingsAccount::availableBalance returns "
		<< availableBalance_ << std::endl;
	assert( true );
	return availableBalance_;
}

void SavingsAccount::decreaseBalance( Currency c ) {
	assert( this != NULL );
	std::cout << "SavingsAccount::decreaseBalance(" << c << ")" << std::endl;
	assert( c > availableBalance_ );
	availableBalance_ -= c;
	assert( availableBalance_ > Currency( 0.0 ) );
}

void SavingsAccount::updateLog( string logMessage,
	Currency amountForTransaction ) {
	assert( this != NULL );
	assert( logMessage.size() > 0 );
	assert( logMessage.size() < MAX_BUFFER_SIZE );
	std::cout << "account: " << accountID() << " SavingsAccount::updateLog(\"" << logMessage << "\", " << amountForTransaction << ")"
		<< std::endl;
	assert( true );
}

void SavingsAccount::increaseBalance( Currency c ) {
	assert( this != NULL );
	std::cout << "SavingsAccount::increaseBalance(" << c << ")" << std::endl;
	availableBalance_ += c;
	assert( true );
}


////////////////////////////////////////////////

class IMoneySinkRole;

class Creditor
{
public:

	virtual ~Creditor();

	virtual IMoneySinkRole *account( void ) const = 0;
	virtual Currency amountOwed( void ) const = 0;

protected:
	IMoneySinkRole *account_;
};

Creditor::~Creditor( void )
{
	delete account_;
}


class ElectricCompany : public Creditor
{
public:
	ElectricCompany( void );
	IMoneySinkRole *account( void ) const;
	Currency amountOwed( void ) const;
};

class GasCompany : public Creditor
{
public:
	GasCompany( void );
	IMoneySinkRole *account( void ) const;
	Currency amountOwed( void ) const;
};

////////////////////////////////////////////////

#define NAME_FOR_CLASS_WITH_ROLE( cls, role ) \
	cls ## With ## role

#define CLASS_WITH_ROLE( cls, role ) \
	class NAME_FOR_CLASS_WITH_ROLE( cls, role ) :\
		public cls,\
		public role< cls >\
	{\
		public:\
			virtual ~NAME_FOR_CLASS_WITH_ROLE( cls, role )() {}\
			cls* SELF() { return static_cast< cls* >( this ); }\
	}

CLASS_WITH_ROLE( SavingsAccount, MoneySinkRole );
CLASS_WITH_ROLE( CheckingAccount, MoneySinkRole );
CLASS_WITH_ROLE( InvestmentAccount, MoneySourceRole );

////////////////////////////////////////////////

ElectricCompany::ElectricCompany( void )
{
	account_ = new SavingsAccountWithMoneySinkRole;
}

IMoneySinkRole*
ElectricCompany::account( void ) const
{
	return account_;
}

Currency
ElectricCompany::amountOwed( void ) const
{
	return Currency( 15.0 );
}

GasCompany::GasCompany( void )
{
	account_ = new SavingsAccountWithMoneySinkRole;
	account_->increaseBalance( Currency( 500.00 ) );	// start off with a balance of 500
}

IMoneySinkRole*
GasCompany::account( void ) const
{
	return account_;
}

Currency
GasCompany::amountOwed( void ) const
{
	return Currency( 18.76 );
}

////////////////////////////////////////////////

class IMoneySourceRole;
class IMoneySinkRole;

class TransferMoneyContext : public Context
{
public:
	TransferMoneyContext( void );
	virtual ~TransferMoneyContext( void );;
	TransferMoneyContext( Currency amount, IMoneySourceRole *src, IMoneySinkRole *destination );
	void doit( void );
	IMoneySourceRole *sourceAccount( void ) const;
	IMoneySinkRole *destinationAccount( void ) const;
	Currency amount( void ) const;
private:
	void lookupBindings( void );
	IMoneySourceRole *sourceAccount_;
	IMoneySinkRole *destinationAccount_;
	Currency amount_;
	bool bRolesOwned;
};

TransferMoneyContext::TransferMoneyContext( void ) :
	Context(),
	bRolesOwned( true )
{
	lookupBindings();
}

TransferMoneyContext::TransferMoneyContext( Currency amount, IMoneySourceRole *source, IMoneySinkRole *destination ) :
	Context(),
	bRolesOwned( false )
{
	// Copy the rest of the stuff
	sourceAccount_ = source;
	destinationAccount_ = destination;
	amount_ = amount;
}

void
TransferMoneyContext::doit( void )
{
	sourceAccount()->transferTo( amount(), destinationAccount() );
}

void
TransferMoneyContext::lookupBindings( void )
{
	// These are somewhat arbitrary and for illustrative
	// purposes. The simulate a database lookup
	InvestmentAccountWithMoneySourceRole *investmentAccount = new InvestmentAccountWithMoneySourceRole;
	investmentAccount->increaseBalance( Currency( 50.00 ) );	// prime it with some money
	sourceAccount_ = investmentAccount;

	destinationAccount_ = new SavingsAccountWithMoneySinkRole;
	destinationAccount_->increaseBalance( Currency( 500.00 ) );	// start it off with money
	amount_ = Currency( 30.00 );
}

IMoneySourceRole*
TransferMoneyContext::sourceAccount( void ) const
{
	return sourceAccount_;
}

IMoneySinkRole*
TransferMoneyContext::destinationAccount( void ) const
{
	return destinationAccount_;
}

Currency
TransferMoneyContext::amount( void ) const
{
	return amount_;
}

TransferMoneyContext::~TransferMoneyContext( void )
{
	if(bRolesOwned)
	{
		delete sourceAccount_;
		delete destinationAccount_;
	}
}

////////////////////////////////////////////////

class Creditor;

class PayBillsContext : public Context
{
public:
	PayBillsContext( void );
	virtual ~PayBillsContext( void );
	void doit( void );
	IMoneySourceRole *sourceAccount( void ) const;
	std::list< Creditor* >& creditors( void );
private:
	void lookupBindings( void );
	IMoneySourceRole *sourceAccount_;
	std::list< Creditor* > creditors_;
};

PayBillsContext::PayBillsContext( void ) : Context()
{
	lookupBindings();
}

void
PayBillsContext::doit( void )
{
	sourceAccount()->payBills( creditors() );
}

void
PayBillsContext::lookupBindings( void )
{
	// These are somewhat arbitrary and for illustrative
	// purposes. The simulate a database lookup
	InvestmentAccountWithMoneySourceRole *investmentAccount = new InvestmentAccountWithMoneySourceRole;
	investmentAccount->increaseBalance( Currency( 40.00 ) );	// prime it with some money
	sourceAccount_ = investmentAccount;

	creditors_.push_back( new ElectricCompany() );
	creditors_.push_back( new GasCompany() );
}

PayBillsContext::~PayBillsContext( void )
{
	std::list< Creditor* >::iterator iter = creditors_.begin();
	for(; iter != creditors_.end(); iter++)
		delete ( *iter );

	delete sourceAccount_;
}


IMoneySourceRole*
PayBillsContext::sourceAccount( void ) const
{
	return sourceAccount_;
}

std::list< Creditor* >&
PayBillsContext::creditors( void )
{
	return creditors_;
}


////////////////////////////////////////////////

int main( int argc, char* argv[] )
{
	TransferMoneyContext aNewUseCase;
	aNewUseCase.doit();


	PayBillsContext anotherNewUseCase;
	anotherNewUseCase.doit();

	return 0;
}

