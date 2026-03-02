export default `import React, { useState } from 'react';

const Product = ({ id, title, description, url, votes, submitterAvatarUrl, productImageUrl, onVote }) => {
  return (
    <div className='flex mb-4 p-4 border rounded shadow-sm bg-white'>
      <div className='w-40 mr-4'>
        <img src={productImageUrl} alt={title} className='w-full rounded' />
      </div>
      <div className='flex-1'>
        <div className='flex items-center mb-1'>
          <button onClick={() => onVote(id)} className='text-blue-500 mr-2'>
            ▲
          </button>
          <span className='font-bold'>{votes}</span>
        </div>
        <div className='mb-2'>
          <a href={url} className='text-blue-600 font-bold hover:underline'>{title}</a>
          <p className='text-gray-600'>{description}</p>
        </div>
        <div className='flex items-center text-sm text-gray-500'>
          <span>Отправлено:</span>
          <img src={submitterAvatarUrl} className='w-6 h-6 rounded-full mx-2' alt="avatar" />
        </div>
      </div>
    </div>
  );
};

const ProductList = () => {
  const [products, setProducts] = useState([
    {
      id: 1,
      title: 'Yellow Pail',
      description: 'On-demand sand castle construction expertise.',
      url: '#',
      votes: 16,
      submitterAvatarUrl: 'https://i.pravatar.cc/150?u=1',
      productImageUrl: 'https://placehold.co/400x300?text=Yellow+Pail',
    },
    {
      id: 2,
      title: 'Super Power Blaster 2000',
      description: 'The ultimate water gun for summer fun.',
      url: '#',
      votes: 12,
      submitterAvatarUrl: 'https://i.pravatar.cc/150?u=2',
      productImageUrl: 'https://placehold.co/400x300?text=Water+Gun',
    }
  ]);

  const handleProductUpVote = (productId) => {
    const nextProducts = products.map((product) => {
      if (product.id === productId) {
        return { ...product, votes: product.votes + 1 };
      } else {
        return product;
      }
    });
    setProducts(nextProducts.sort((a, b) => b.votes - a.votes));
  };

  const sortedProducts = products.sort((a, b) => b.votes - a.votes);

  return (
    <div className='max-w-2xl mx-auto p-4'>
      <h1 className='text-3xl font-bold mb-6 text-center border-b pb-4'>Популярные продукты</h1>
      {sortedProducts.map((product) => (
        <Product
          key={'product-' + product.id}
          {...product}
          onVote={handleProductUpVote}
        />
      ))}
    </div>
  );
};

export default ProductList;`;
