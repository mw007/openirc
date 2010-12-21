#include "ircclient.h"
#include "pcrecpp.h"

using namespace std;

int IRCClient::Cid = 0;

IRCClient::IRCClient(QObject *parent, const QString &server, const int port)
  : QTcpSocket(parent), ParseLine("^(?:\\x3a(\\S+) )?(\\d{3}|[a-zA-Z]+)(?: ((?:[^\\x00\\x0a\\x0d\\x20\\x3a][^\\x00\\x0a\\x0d\\x20]*)(?: [^\\x00\\x0a\\x0d\\x20\\x3a][^\\x00\\x0a\\x0d\\x20]*)*))? ?(?:\\x3a([^\\x00\\x0a\\x0d]*))?\\x20*$"), ParseJoin("(?:^| )([\\@\\+\\-]+)?([^! ]+)!?([^ ]+)?"), NickOrServer("^[^!@]+$"), NickUser("^([^!]+)!(.*)$"), IsChan("^\\#"), IsAction("^ACTION (.*)$")

{
	this->cid = Cid++;

	 // Connect the signals and slots
	connect(this, SIGNAL(readyRead()), this, SLOT(dataReceived()));
	connect(this, SIGNAL(connected()), this, SLOT(connectedToIRCHost()));
	connect(this, SIGNAL(disconnected()), this, SLOT(disconnectedFromIRCHost()));
	//connect(this, SIGNAL(error(SocketError)), this, SLOT(networkError(SocketError)));

	 if (server != 0) {
		 // TODO: Connect to the server
	 }
}

void IRCClient::changeNick(const QString &newNick)
{
	QString changeNickString("NICK ");
	changeNickString.append(newNick);

	this->write(changeNickString.toAscii());
}

void IRCClient::sendRawMessage(const QString &rawMessage)
{
	QString message = QString(rawMessage);
	message.append("\n");
	this->write(message.toAscii());
}

void IRCClient::connectedToIRCHost()
{
	emit connected(this);
}

void IRCClient::disconnectedFromIRCHost()
{
	emit disconnected(this);
}

void IRCClient::networkError(SocketError error)
{
	emit ircError(this, error);
}

void IRCClient::dataReceived()
{
	while (this->canReadLine()) {
		line = this->readLine();
		line.remove('\r');
		line.remove('\n');
		if (line.isEmpty()) { return; }
		else {
  			if (ParseLine.FullMatch(line.toStdString(), &NorS, &Event, &args, &Extra)) {
			//qDebug() << "Split Data: NorS(" + QString::fromStdString(NorS) + ") Event(" + QString::fromStdString(Event) + ") args(" + QString::fromStdString(args) + ") Extra(" + QString::fromStdString(Extra) + ")";
				Args.clear();
				Args = QString::fromStdString(args).split(" ");
				if (NorS == "") {
	    				if (Event == "PING") { this->sendRawMessage(QString::fromStdString("PONG " + Extra)); }
				}
				else {
					if (NickOrServer.PartialMatch(NorS)) {
						/*
						Server Message
						trigger RAW event
						*/
						if (Event == "001") {
							//set server NorS
							//find out our userhost info
							this->sendRawMessage(QString::fromStdString("USERHOST " + Me));
							Me = Args.at(0).toStdString();
						}
						/*
						SERIOUS parsing of 005 for chanmodes, chantypes, modes, network, prefix, nickmode,namesx,uhnames...
						else if (Event == "005") {
						}
						*/
						else if (Event == "302") {
							//userhost return raw use to set ial of ourself
						}
						else if (Event == "305") {
							//set back from being away
						}
						else if (Event == "306") {
							//we set ourself away
						}
						else if (Event == "353") {
							QString channel = Args.at(Args.size() - 1);
							QStringList nicklist;

							if (this->tempChannelJoins.contains(channel)) {
								nicklist = this->tempChannelJoins[channel];
							}

							pcrecpp::StringPiece ExData(Extra);
							while (ParseJoin.Consume(&ExData,&JoinPrefix,&JoinNick,&JoinAddress)) {
								nicklist << QString::fromStdString(JoinNick);
							//	emit channelJoined(this,QString::fromStdString(Args.at(Args.size() - 1)),QString::fromStdString(JoinNick));
							}

							if (!this->tempChannelJoins.contains(channel)) {
								this->tempChannelJoins.insert(channel, nicklist);
							}
						} else if (Event == "366") {
							QString channel = Args.at(Args.size() - 1);

							if (this->tempChannelJoins.contains(channel)) {
								QStringList nicklist = this->tempChannelJoins.take(channel);
								emit channelJoinCompleteNickList(this, channel, nicklist);
							}
						}
						emit incomingData(this, line);
					}
					else {
						//Nickname Message
						string Nick;
						string Address;
						NickUser.PartialMatch(NorS, &Nick, &Address);
						if (Event == "JOIN" || Event == "PART") {
							emit channelMessageReceived(this, (IsChan.PartialMatch(Extra) > 0 ? QString::fromStdString(Extra) : Args.at(0)),QString::fromStdString(Event),QString::fromStdString(Nick),QString::fromStdString(Address),QString::fromStdString(Extra));
							if (Event == "JOIN") {
								emit channelJoined(this,(IsChan.PartialMatch(Extra) > 0 ? QString::fromStdString(Extra) : Args.at(0)),QString::fromStdString(Nick));
							}
							else {
								emit channelParted(this,(IsChan.PartialMatch(Extra) > 0 ? QString::fromStdString(Extra) : Args.at(0)),QString::fromStdString(Nick));
							}
						}
						else if (Event == "PRIVMSG") {
							if (IsChan.PartialMatch(Args.at(0).toStdString())) {
								if (IsAction.PartialMatch(Extra,&Action)) {
									emit channelMessageReceived(this, Args.at(0),QString::fromStdString("ACTION"),QString::fromStdString(Nick),QString::fromStdString(Address),QString::fromStdString(Action));
								}
								else {
									emit channelMessageReceived(this, Args.at(0),QString::fromStdString(Event),QString::fromStdString(Nick),QString::fromStdString(Address),QString::fromStdString(Extra));
								}
							}
							else { 
								emit privateMessageReceived(this, QString::fromStdString(Nick),QString::fromStdString(Address),QString::fromStdString(Extra));
							}
						}
						else { emit incomingData(this, line); }
					}
				}
			}
		}
	}
	//QByteArray data = this->readAll();
	//emit incomingData(this, QString(data));
}

