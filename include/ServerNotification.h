#ifndef SERVERNOTIFICATION_H
#define SERVERNOTIFICATION_H

class ServerNotification
{
	public:
		enum ServerNotificationType {turnStarted};

		ServerNotificationType type;
};

#endif

