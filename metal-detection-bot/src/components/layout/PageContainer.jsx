import React from 'react';
import { ScrollArea } from '../ui/scroll-area';
import Header from './Header';

const PageContainer = ({ children, scrollable = false }) => {
  return (
    <div className='flex flex-col min-h-screen bg-background'>
      <Header />
      <main className='flex-1 overflow-hidden h-full'>
        {scrollable ? (
          <ScrollArea className='h-[calc(100dvh-70px)] px-4 md:px-6'>
            <div className='max-w-3xl mx-auto'>
              {children}
            </div>
          </ScrollArea>
        ) : (
          <div className='h-full px-4 md:px-6'>
            <div className='max-w-7xl mx-auto h-full'>
              {children}
            </div>
          </div>
        )}
      </main>
    </div>
  );
};

export default PageContainer;
