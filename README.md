# Code-DCI

DCI is an example of Data-Context-Interaction architecture, originally coined by James Coplien and Trygve Reenskaug.

**Data** classes represent *domain objects* or entities. Their interface is generic and independent from any use case. For example, an Account (with derived classes such as SavingsAccount or InvestmentAccount) and a Creditor (with derived classes such as GasCompany or ElectricCompany) could be Data classes.

**Interaction** classes represent object *roles* that dynamically extend the entities' behavior with the *use case* specific functionality. For example, MoneySource and MoneySink could be Role classes for Account objects.

**Context** classes are the places where data objects are retrieved and roles assigned. Ideally, a Context holds references to all participants via their roles in a single use case, and does nothing but calls each one by their roles. For example, PayBillsContext could be a Context class that binds an Account, GasCompany and ElectricCompany to the PayBills use case.
