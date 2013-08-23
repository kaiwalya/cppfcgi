//
//  logic.h
//  fcgi
//
//  Created by Kaiwalya Kher on 8/17/13.
//  Copyright (c) 2013 Kaiwalya Kher. All rights reserved.
//

#ifndef fcgi_logic_h
#define fcgi_logic_h

namespace komm
{
	namespace logic
	{
		template<typename T>
		class singleton
		{
		private:
			static T * m_globalObject;
		public:
			static T & instance()
			{
				if (!m_globalObject)
					m_globalObject = new T();
				return *m_globalObject;
			}
			
			static void destroy()
			{
				delete m_globalObject;
				m_globalObject = nullptr;
			}
		};
	}
}

#endif
