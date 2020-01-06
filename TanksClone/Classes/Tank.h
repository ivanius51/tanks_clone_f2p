#ifndef IMovableObject_
#define IMovableObject_
#include "IMovableObject.h"

NS_CC_BEGIN
class Tank : public IMovableObject
{
private:
	virtual bool build() override;

	virtual void onCommandReceived(GameCommand aCommand) override;
public:
	Tank();
	~Tank();

	static Tank* create(Node* aNode);
};

NS_CC_END

#endif