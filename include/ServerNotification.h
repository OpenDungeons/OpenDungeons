#ifndef SERVERNOTIFICATION_H
#define SERVERNOTIFICATION_H

/*! \brief A data structure used to pass messages to the serverNotificationProcessor thread.
 *
 */
class ServerNotification
{
	public:
		enum ServerNotificationType {turnStarted};

		ServerNotificationType type;
};

#endif

