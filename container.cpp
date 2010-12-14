#include "container.h"

#include "statuswindow.h"
#include "channelwindow.h"
#include "querywindow.h"

Container::Container(QWidget *parent, Qt::WindowFlags flags)
  : QMainWindow(parent, flags)
{
	// Create the dock window
	//this->contextBar = new ContextBar(this, 0);
	//this->addDockWidget(Qt::TopDockWidgetArea, this->contextBar);

	// Create the mdi area
	this->mdiArea = new QMdiArea(this);
	this->setCentralWidget(this->mdiArea);

	this->newWindow();
}

void Container::newWindow()
{
	StatusWindow *window = new StatusWindow(this->mdiArea);
	window->setTitle(tr("Not Connected"));

	ChannelWindow *channelWindow = new ChannelWindow(this->mdiArea);
	channelWindow->setTitle(tr("#Channel Window"));

	QueryWindow *queryWindow = new QueryWindow(this->mdiArea);
	queryWindow->setTitle(tr("@Query Window"));
}
